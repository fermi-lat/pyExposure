# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/ScienceTools-scons/pyExposure/SConscript,v 1.2 2007/12/11 19:41:39 jchiang Exp $
Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('pyExposureLib', depsOnly = 1)
lib_pyExposureStaticLib = libEnv.StaticLibrary('lib_pyExposure', 
                                               'src/pyExposure.i')
lib_pyExposureSharedLib = libEnv.SharedLibrary('lib_pyExposure', 
                                               'src/pyExposure.i')
pyExposureLib = libEnv.StaticLibrary('pyExposure', listFiles(['src/*.cxx']))

pyExposureSharedLib = libEnv.SharedLibrary('pyExposure', 
                                           listFiles(['src/*.cxx']))

progEnv.Tool('pyExposureLib')

gtexposureBin = progEnv.Program('gtexposure', 
                                listFiles(['src/gtexposure/*.cxx']))

progEnv.Tool('registerObjects', package = 'pyExposure',
             libraries = [pyExposureLib, pyExposureSharedLib,
                          lib_pyExposureStaticLib, lib_pyExposureSharedLib],
             binaries = [gtexposureBin],
             includes = listFiles(['pyExposure/*.h']), 
             pfiles = listFiles(['pfiles/*.par']))
