/**
 * @file gtexposure.cxx
 * @brief Calculate exposure as a function of time at a specific location
 * on the sky as given by an LC file created by gtbin and add the exposure
 * column to the LC file.
 *
 * @author J. Chiang
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/ScienceTools-scons/pyExposure/src/gtexposure/gtexposure.cxx,v 1.10 2013/10/10 18:59:11 jchiang Exp $
 */

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "xmlBase/rapidxml.hpp"

#include "st_stream/StreamFormatter.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "tip/Header.h"
#include "tip/IFileSvc.h"
#include "tip/Table.h"

#include "st_facilities/Util.h"

#include "optimizers/dArg.h"
#include "optimizers/Function.h"
#include "optimizers/FunctionFactory.h"

#include "dataSubselector/Cuts.h"
#include "dataSubselector/Gti.h"
#include "dataSubselector/GtiCut.h"
#include "dataSubselector/RangeCut.h"
#include "dataSubselector/SkyConeCut.h"

#include "Likelihood/BrokenPowerLawExpCutoff.h"
#include "Likelihood/BrokenPowerLaw2.h"
#include "Likelihood/ExpCutoff.h"
#include "Likelihood/FileFunction.h"
#include "Likelihood/LogParabola.h"
#include "Likelihood/MapCubeFunction2.h"
#include "Likelihood/PowerLaw2.h"
#include "Likelihood/PowerLawSuperExpCutoff.h"
#include "Likelihood/PowerLawSuperExpCutoff2.h"
#include "Likelihood/PowerLawSuperExpCutoff3.h"
#include "Likelihood/PowerLawSuperExpCutoff4.h"

#include "pyExposure/Exposure.h"

namespace {
   // Helper function to get attribute value from a RapidXML node
   std::string getAttribute(rapidxml::xml_node<>* node, const char* attrName) {
      if (!node) {
         return "";
      }
      auto* attr = node->first_attribute(attrName);
      if (attr && attr->value()) {
         return std::string(attr->value());
      }
      return "";
   }

   // Helper function to check tag name
   bool checkTagName(rapidxml::xml_node<>* node, const char* tagName) {
      if (!node || !node->name()) {
         return false;
      }
      return std::string(node->name()) == tagName;
   }

   // Helper function to collect children by tag name
   void getChildrenByTagName(rapidxml::xml_node<>* parent, 
                             const char* tagName,
                             std::vector<rapidxml::xml_node<>*>& children) {
      children.clear();
      if (!parent) {
         return;
      }
      for (auto* child = parent->first_node(tagName); 
           child != nullptr; 
           child = child->next_sibling(tagName)) {
         children.push_back(child);
      }
   }

   // Helper function to read file contents
   std::string readFileContents(const std::string& filename) {
      std::ifstream file(filename);
      if (!file) {
         throw std::runtime_error("Cannot open file: " + filename);
      }
      std::ostringstream ss;
      ss << file.rdbuf();
      return ss.str();
   }
}

class GtExposure : public st_app::StApp {

public:

   GtExposure();

   ~GtExposure() noexcept override {
      try {
         // Use unique_ptr members - cleanup is automatic
      } catch (const std::exception& eObj) {
         std::cout << eObj.what() << std::endl;
      } catch (...) {
      }
   }

   void run() override;

   void banner() const override;

private:

   st_app::AppParGroup& m_pars;
   std::unique_ptr<st_stream::StreamFormatter> m_formatter;
   std::unique_ptr<optimizers::FunctionFactory> m_funcFactory;
   std::unique_ptr<pyExposure::Exposure> m_exposure;
   std::unique_ptr<optimizers::Function> m_function;

   double m_emin{0.0};
   double m_emax{0.0};
   double m_ra{0.0};
   double m_dec{0.0};
   double m_radius{180.0};

   std::vector<double> m_weightedExps;

   static std::string s_cvs_id;

   void prepareFunctionFactory();
   void promptForParameters();
   void parseDssKeywords();
   void setExposure();
   void getLcTimes(std::vector<double>& tlims) const;
   void prepareModel();
   void performSpectralWeighting();
   void writeExposure();
};

st_app::StAppFactory<GtExposure> myAppFactory("gtexposure");

std::string GtExposure::s_cvs_id("$Name:  $");

GtExposure::GtExposure() 
   : st_app::StApp(), 
     m_pars(st_app::StApp::getParGroup("gtexposure")),
     m_formatter(std::make_unique<st_stream::StreamFormatter>("gtexposure", "", 2)),
     m_funcFactory(std::make_unique<optimizers::FunctionFactory>()) {
   setVersion(s_cvs_id);
   prepareFunctionFactory();
}

void GtExposure::banner() const {
   int verbosity = m_pars["chatter"];
   if (verbosity > 2) {
      st_app::StApp::banner();
   }
}

void GtExposure::run() {
   promptForParameters();
   parseDssKeywords();
   setExposure();
   prepareModel();
   performSpectralWeighting();
   writeExposure();
}

void GtExposure::promptForParameters() {
   m_pars.Prompt("infile");
   m_pars.Prompt("scfile");
   m_pars.Prompt("irfs");
   m_pars.Prompt("srcmdl");
   std::string xmlFile = m_pars["srcmdl"];
   if (xmlFile == "none") {
      m_pars.Prompt("specin");
   } else {
      m_pars.Prompt("target");
   }
   m_pars.Save();
}

void GtExposure::parseDssKeywords() {
   std::string lc_file = m_pars["infile"];
   bool aperture_correct = m_pars["apcorr"];
   dataSubselector::Cuts cuts(lc_file, "RATE", false);
   m_ra = m_pars["ra"];
   m_dec = m_pars["dec"];
   if (aperture_correct) {
      m_radius = m_pars["rad"];
   }
   m_emin = m_pars["emin"];
   m_emax = m_pars["emax"];
   for (size_t i = 0; i < cuts.size(); ++i) {
      if (cuts[i].type() == "SkyCone") {
         const auto& my_cut = dynamic_cast<dataSubselector::SkyConeCut&>(
               const_cast<dataSubselector::CutBase&>(cuts[i]));
         m_ra = my_cut.ra();
         m_dec = my_cut.dec();
         if (aperture_correct) {
            m_radius = my_cut.radius();
         }
      }
      if (cuts[i].type() == "range") {
         const auto& my_cut = dynamic_cast<dataSubselector::RangeCut&>(
               const_cast<dataSubselector::CutBase&>(cuts[i]));
         if (my_cut.colname() == "ENERGY") {
            m_emin = my_cut.minVal();
            m_emax = my_cut.maxVal();
         }
      }
   }
}

void GtExposure::setExposure() {
   std::vector<double> energies;
   int nee = m_pars["enumbins"];
   double estep = std::log(m_emax / m_emin) / (nee - 1);
   for (int k = 0; k < nee; ++k) {
      energies.push_back(m_emin * std::exp(estep * k));
   }
   std::vector<double> tlims;
   getLcTimes(tlims);
   std::string lc_file = m_pars["infile"];
   std::string ft2file = m_pars["scfile"];
   std::string irfs = m_pars["irfs"];
   if (irfs == "CALDB") {
      dataSubselector::Cuts cuts(lc_file, "RATE", false);
      irfs = cuts.CALDB_implied_irfs();
   }
   dataSubselector::GtiCut gtiCut(lc_file);
   std::vector<std::pair<double, double>> gtis;
   for (auto it = gtiCut.gti().begin(); it != gtiCut.gti().end(); ++it) {
      gtis.emplace_back(it->first, it->second);
   }
   m_exposure = std::make_unique<pyExposure::Exposure>(
      ft2file, tlims, gtis, energies, m_ra, m_dec, m_radius, irfs);
}

void GtExposure::getLcTimes(std::vector<double>& tlims) const {
   std::string lc_file = m_pars["infile"];
   const tip::Table* table = 
      tip::IFileSvc::instance().readTable(lc_file, "RATE");

   tip::Table::ConstIterator it(table->begin());
   const tip::ConstTableRecord& row(*it);

   double time = 0.0;
   double dt = 0.0;
   tlims.clear();
   for (; it != table->end(); ++it) {
      row["TIME"].get(time);
      row["TIMEDEL"].get(dt);
      tlims.push_back(time - dt / 2.0);
   }
   tlims.push_back(time + dt / 2.0);

   delete table;
}

void GtExposure::prepareFunctionFactory() {
   m_funcFactory->addFunc("BPLExpCutoff",
                          new Likelihood::BrokenPowerLawExpCutoff(), 
                          false);
   m_funcFactory->addFunc("BrokenPowerLaw2", 
                          new Likelihood::BrokenPowerLaw2(),
                          false);
   m_funcFactory->addFunc("ExpCutoff", 
                          new Likelihood::ExpCutoff(), 
                          false);
   m_funcFactory->addFunc("LogParabola", 
                          new Likelihood::LogParabola(), 
                          false);
   m_funcFactory->addFunc("FileFunction", 
                          new Likelihood::FileFunction(), 
                          false);
   m_funcFactory->addFunc("MapCubeFunction", 
                          new Likelihood::MapCubeFunction2(),
                          false);
   m_funcFactory->addFunc("PowerLaw2", 
                          new Likelihood::PowerLaw2(), 
                          false);
   m_funcFactory->addFunc("PLSuperExpCutoff", 
                          new Likelihood::PowerLawSuperExpCutoff(), 
                          false);
   m_funcFactory->addFunc("PLSuperExpCutoff2", 
                          new Likelihood::PowerLawSuperExpCutoff2(), 
                          false);
   m_funcFactory->addFunc("PLSuperExpCutoff3", 
                          new Likelihood::PowerLawSuperExpCutoff3(), 
                          false);
   m_funcFactory->addFunc("PLSuperExpCutoff4", 
                          new Likelihood::PowerLawSuperExpCutoff4(), 
                          false);
}

void GtExposure::prepareModel() {
   std::string xmlFile = m_pars["srcmdl"];
   std::string srcName = m_pars["target"];
   double gamma = m_pars["specin"];
   
   if (xmlFile == "none") {
      m_function.reset(m_funcFactory->create("PowerLaw2"));
      m_function->setParam("Index", gamma);
      return;
   }

   // Read the XML file contents
   std::string xmlContent = readFileContents(xmlFile);
   
   // RapidXML requires non-const char* for in-place parsing
   std::vector<char> xmlBuffer(xmlContent.begin(), xmlContent.end());
   xmlBuffer.push_back('\0');
   
   rapidxml::xml_document<> doc;
   try {
      doc.parse<rapidxml::parse_default>(xmlBuffer.data());
   } catch (const rapidxml::parse_error& e) {
      throw std::runtime_error("XML parse error in " + xmlFile + ": " + e.what());
   }
   
   auto* source_library = doc.first_node();
   if (!checkTagName(source_library, "source_library")) {
      throw std::runtime_error("source_library not found in " + xmlFile);
   }
   
   std::vector<rapidxml::xml_node<>*> srcs;
   getChildrenByTagName(source_library, "source", srcs);
   
   for (auto* srcNode : srcs) {
      std::string name = getAttribute(srcNode, "name");
      if (name == srcName) {
         std::vector<rapidxml::xml_node<>*> children;
         getChildrenByTagName(srcNode, "spectrum", children);
         if (children.empty()) {
            throw std::runtime_error("No spectrum element found for source " + srcName);
         }
         auto* spectrum = children.front();
         std::string type = getAttribute(spectrum, "type");
         m_function.reset(m_funcFactory->create(type));
         m_function->setParams(spectrum);
         return;
      }
   }
   throw std::runtime_error("Source named " + srcName + " not found in " + xmlFile);
}

void GtExposure::performSpectralWeighting() {
   const auto& energies = m_exposure->energies();
   std::vector<double> dndes;
   dndes.reserve(energies.size());
   
   for (const auto& energy : energies) {
      optimizers::dArg arg(energy);
      dndes.push_back(m_function->operator()(arg));
   }
   
   double dnde_int = 0.0;
   for (size_t k = 0; k < energies.size() - 1; ++k) {
      dnde_int += ((dndes[k + 1] + dndes[k]) / 2.0
                   * (energies[k + 1] - energies[k]));
   }
   
   const auto& exposures = m_exposure->values();
   m_weightedExps.clear();
   m_weightedExps.reserve(exposures.size());
   
   for (const auto& row : exposures) {
      double my_exposure = 0.0;
      for (size_t k = 0; k < energies.size() - 1; ++k) {
         my_exposure += ((row[k + 1] * dndes[k + 1] 
                         + row[k] * dndes[k]) / 2.0
                        * (energies[k + 1] - energies[k])) / dnde_int;
      }
      m_weightedExps.push_back(my_exposure);
   }
}

void GtExposure::writeExposure() {
   std::string lc_file = m_pars["infile"];
   tip::Table* table =
      tip::IFileSvc::instance().editTable(lc_file, "RATE");
   
   try {
      table->appendField("EXPOSURE", "E");
      tip::Header& header(table->getHeader());
      std::ostringstream unit_label;
      unit_label << "TUNIT" << table->getFieldIndex("EXPOSURE") + 1;
      header[unit_label.str()].set("cm**2 s");
   } catch (const tip::TipException& eObj) {
      if (!st_facilities::Util::expectedException(eObj, "already exists")) {
         throw;
      }
   }
   
   tip::Table::Iterator it(table->begin());
   tip::TableRecord& row(*it);

   if (m_weightedExps.size() != static_cast<size_t>(table->getNumRecords())) {
      throw std::runtime_error("Size of exposures does not equal size of "
                               "lc file RATE table");
   }

   for (size_t i = 0; it != table->end(); ++it, ++i) {
      row["exposure"].set(m_weightedExps[i]);
   }
   delete table;
}
