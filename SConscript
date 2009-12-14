# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/pyExposure/SConscript,v 1.22 2009/12/01 20:51:25 jchiang Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-05-04

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()
#swigEnv = baseEnv.Clone()

libEnv.Tool('pyExposureLib', depsOnly = 1)
#pyExposureLib = libEnv.StaticLibrary('pyExposure', listFiles(['src/*.cxx']))
pyExposureLib = libEnv.SharedLibrary('pyExposure', listFiles(['src/*.cxx']))

#pyExposureSharedLib = libEnv.SharedLibrary('pyExposure', 
#                                           listFiles(['src/*.cxx']))

#swigEnv.Tool('pyExposureLib')
#lib_pyExposureSharedLib = swigEnv.LoadableModule('_pyExposure', 
#                                                 'src/pyExposure.i')

progEnv.Tool('pyExposureLib')

gtexposureBin = progEnv.Program('gtexposure', 
                                listFiles(['src/gtexposure/*.cxx']))

progEnv.Tool('registerTargets', package = 'pyExposure',
             #staticLibraryCxts=[[pyExposureLib, libEnv]],
             libraryCxts=[[pyExposureLib, libEnv]],
             #swigLibraryCxts=[[lib_pyExposureSharedLib, swigEnv]],
             binaryCxts = [[gtexposureBin, progEnv]],
             includes = listFiles(['pyExposure/*.h']), 
             pfiles = listFiles(['pfiles/*.par']))
             #python = ['src/pyExposure.py'])
