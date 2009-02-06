# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/ScienceTools-scons/pyExposure/SConscript,v 1.11 2008/12/12 01:30:45 glastrm Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-03-00

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()
swigEnv = baseEnv.Clone()

libEnv.Tool('pyExposureLib', depsOnly = 1)
pyExposureLib = libEnv.StaticLibrary('pyExposure', listFiles(['src/*.cxx']))

pyExposureSharedLib = libEnv.SharedLibrary('pyExposure', 
                                           listFiles(['src/*.cxx']))

swigEnv.Tool('pyExposureLib')
lib_pyExposureStaticLib = swigEnv.StaticLibrary('_pyExposure', 
                                               'src/pyExposure.i', LIBPREFIX='')
lib_pyExposureSharedLib = swigEnv.SharedLibrary('_pyExposure', 
                                               'src/pyExposure.i', SHLIBPREFIX='')

progEnv.Tool('pyExposureLib')

gtexposureBin = progEnv.Program('gtexposure', 
                                listFiles(['src/gtexposure/*.cxx']))

progEnv.Tool('registerObjects', package = 'pyExposure',
             libraries = [pyExposureLib, pyExposureSharedLib,
                          lib_pyExposureStaticLib, lib_pyExposureSharedLib],
             binaries = [gtexposureBin],
             includes = listFiles(['pyExposure/*.h']), 
             pfiles = listFiles(['pfiles/*.par']))
