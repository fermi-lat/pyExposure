# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/pyExposure/SConscript,v 1.34 2013/10/10 18:59:50 jchiang Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-06-01

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()
#swigEnv = baseEnv.Clone()

if baseEnv['PLATFORM'] == "posix":
    libEnv.Append(CPPDEFINES = 'TRAP_FPE')

libEnv.Tool('addLinkDeps', package='pyExposure', toBuild='shared')

pyExposureLib = libEnv.SharedLibrary('pyExposure', listFiles(['src/*.cxx']))


#swigEnv.Tool('pyExposureLib')
#lib_pyExposureSharedLib = swigEnv.LoadableModule('_pyExposure', 
#                                                 'src/pyExposure.i')


progEnv.Tool('pyExposureLib')
if baseEnv['PLATFORM'] == "posix":
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
