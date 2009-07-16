# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/pyExposure/SConscript,v 1.15 2009/07/16 01:02:45 jrb Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-04-01

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
lib_pyExposureSharedLib = swigEnv.LoadableModule('_pyExposure', 
                                                 'src/pyExposure.i')

progEnv.Tool('pyExposureLib')

gtexposureBin = progEnv.Program('gtexposure', 
                                listFiles(['src/gtexposure/*.cxx']))

progEnv.Tool('registerTargets', package = 'pyExposure',
             staticLibraryCxts=[[pyExposureLib, libEnv]],
             libraryCxts=[[pyExposureSharedLib, libEnv]],
             swigLibraryCxts=[[lib_pyExposureSharedLib, swigEnv]],
             binaryCxts = [[gtexposureBin, progEnv]],
             includes = listFiles(['pyExposure/*.h']), 
             pfiles = listFiles(['pfiles/*.par']),
             python = ['src/pyExposure.py'])
