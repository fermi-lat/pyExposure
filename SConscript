# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/pyExposure/SConscript,v 1.7 2008/07/25 21:30:35 glastrm Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-01-01-01

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
lib_pyExposureStaticLib = swigEnv.StaticLibrary('lib_pyExposure', 
                                               'src/pyExposure.i')
lib_pyExposureSharedLib = swigEnv.SharedLibrary('lib_pyExposure', 
                                               'src/pyExposure.i')

progEnv.Tool('pyExposureLib')

gtexposureBin = progEnv.Program('gtexposure', 
                                listFiles(['src/gtexposure/*.cxx']))

progEnv.Tool('registerObjects', package = 'pyExposure',
             libraries = [pyExposureLib, pyExposureSharedLib,
                          lib_pyExposureStaticLib, lib_pyExposureSharedLib],
             binaries = [gtexposureBin],
             includes = listFiles(['pyExposure/*.h']), 
             pfiles = listFiles(['pfiles/*.par']))
