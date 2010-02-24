# -*- python -*-
#
# $Header: /nfs/slac/g/glast/ground/cvs/ScienceTools-scons/pyExposure/SConscript,v 1.27 2010/02/24 20:06:28 jrb Exp $
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: pyExposure-02-05-06

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()
#swigEnv = baseEnv.Clone()

libEnv.Append(CPPDEFINES = 'TRAP_FPE')

libEnv.Tool('addLinkDeps', package='pyExposure', toBuild='shared')

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
