# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/pyExposure/SConscript,v 1.6 2008/07/05 07:30:36 glastrm Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-01-00

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
