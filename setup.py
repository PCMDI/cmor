import numpy
from numpy.distutils.core import setup, Extension
#from numpy.distutils.ccompiler import CCompiler
import os,sys,string

include_dirs = [numpy.lib.utils.get_include(),"include","include/cdTime","include/json-c"]

library_dirs = [ os.path.join("/usr/local/cmor","lib") ,'.']
include_dirs.append(os.path.join("/usr/local/cmor","include"))
libraries = []

for st in ["-L/software/run/uvcdatall/Externals/lib -lnetcdf", "-I/software/run/uvcdatall/Externals/include -I/software/run/uvcdatall/Externals/include -I/software/run/uvcdatall/Externals/lib/libffi-3.1/include",  
           "-ludunits2", "",
           " -I/software/run/uvcdatall//include", " -L/software/run/uvcdatall//lib  -Wl,-rpath=/software/run/uvcdatall//lib -luuid"]:
   sp = st.strip().split()
   for s in sp:
      if s[:2]=='-L':
        library_dirs.append(s[2:])
      if s[:2]=='-l':
        libraries.append(s[2:])
      if s[:2]=='-I':
        include_dirs.append(s[2:])

srcfiles = "Src/_cmormodule.c  Src/cdTime/cdRegTrfm.c  Src/cdTime/cdTimeConv.c  Src/cdTime/cdUtil.c  Src/cdTime/fcdTimeConv.c  Src/cdTime/reg_trfm.c  Src/cdTime/timeArith.c  Src/cdTime/timeConv.c  Src/cmor.c  Src/cmor_axes.c  Src/cmor_cfortran_interface.c  Src/cmor_grids.c  Src/cmor_md5.c  Src/cmor_tables.c  Src/cmor_variables.c	Src/json-c/arraylist.c	Src/json-c/debug.c  Src/json-c/json_c_version.c  Src/json-c/json_object.c  Src/json-c/json_object_iterator.c  Src/json-c/json_tokener.c  Src/json-c/json_util.c  Src/json-c/libjson.c  Src/json-c/linkhash.c  Src/json-c/printbuf.c  Src/json-c/random_seed.c".split()

macros=[]
for m in " -DCOLOREDOUTPUT".split():
   macros.append((m[2:],None))
   
ld =[]
for p in library_dirs:
   if os.path.exists(p):
      ld.append(p)
library_dirs=ld

ld =[]
for p in include_dirs:
   if os.path.exists(p):
      ld.append(p)
include_dirs=ld

print 'Setting up python module with:'
print 'libraries:',libraries
print 'libdir:',library_dirs
print 'incdir',include_dirs
print 'src:',srcfiles
print 'macros:',macros

setup (name = "CMOR",
       version='3.0',
       author='Denis Nadeau, AIMS',
       description = "Python Interface to CMOR output library",
       url = "http://www-pcmdi.llnl.gov/cmor",
       packages = ['cmor'],
       package_dir = {'cmor': 'Lib'},
       ext_modules = [
    Extension('cmor._cmor',
              srcfiles,
	      include_dirs = include_dirs,
              library_dirs = library_dirs,
              libraries = libraries,
              define_macros = macros,
              extra_compile_args = [ "-g", ]
              ),
    
    ]
      )

