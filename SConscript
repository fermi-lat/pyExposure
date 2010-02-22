# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/pyExposure/SConscript,v 1.25 2010/02/22 20:27:20 jrb Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-05-05

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()
#swigEnv = baseEnv.Clone()

libEnv.Append(CPPDEFINES = 'TRAP_FPE')

if baseEnv['PLATFORM'] == "win32":
    libEnv.Tool('pyExposureLib', depsOnly = 1)

pyExposureLib = libEnv.SharedLibrary('pyExposure', listFiles(['src/*.cxx']))


#swigEnv.Tool('pyExposureLib')
#lib_pyExposureSharedLib = swigEnv.LoadableModule('_pyExposure', 
#                                                 'src/pyExposure.i')


progEnv.Tool('pyExposureLib')
progEnv.Append(CPPDEFINES = 'TRAP_FPE')

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
