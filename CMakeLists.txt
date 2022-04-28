add_library(pyExposure SHARED src/Exposure.cxx)
target_include_directories(
  pyExposure
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:>
)

target_link_libraries(
  pyExposure
  PUBLIC astro irfLoader Likelihood st_facilities xmlBase dataSubselector
  dataSubselector st_stream st_app tip optimizers
)

add_executable(gtexposure src/gtexposure/gtexposure.cxx)
target_link_libraries(gtexposure PRIVATE pyExposure)
target_include_directories(
  gtexposure
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  PUBLIC $<INSTALL_INTERFACE:>
)

if(NOT APPLE AND NOT WIN32)
  target_compile_definitions(pyExposure PRIVATE TRAP_FPE)
  target_compile_definitions(gtexposure PRIVATE TRAP_FPE)
endif()

###############################################################
# Installation
###############################################################
install(DIRECTORY pyExposure DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY pfiles/ DESTINATION ${FERMI_INSTALL_PFILESDIR})

install(
  TARGETS pyExposure gtexposure
  EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
