import cmor
import numpy
import sys
import os
from time import localtime, strftime
today = strftime("%Y%m%d", localtime())

try:
    import cdms2
    cdms2.setNetcdfShuffleFlag(0)
    cdms2.setNetcdfDeflateFlag(0)
    cdms2.setNetcdfDeflateLevelFlag(0)
except BaseException:
    print "This test code needs a recent cdms2 interface for i/0"
    sys.exit()

if len(sys.argv) > 1:
    level = int(sys.argv[1])
else:
    level = int(os.environ.get("DEFLATE_LEVEL", 0))

if len(sys.argv) > 2:
    shuffle = int(sys.argv[2])
else:
    shuffle = int(os.environ.get("SHUFFLE", 0))

if level == 0:
    deflate = 0
else:
    deflate = 1

f = open("Test/speed_test_table_A")
s = f.read()
f.close()
s = s.replace("${DEFLATE_LEVEL}", str(level))
s = s.replace("${DEFLATE}", str(deflate))
s = s.replace("${SHUFFLE}", str(shuffle))
f = open("mytable", "w")
f.write(s)
f.close()

cmor.setup(
    inpath="Tables",
    set_verbosity=cmor.CMOR_NORMAL,
    netcdf_file_action=cmor.CMOR_REPLACE_4,
    exit_control=cmor.CMOR_EXIT_ON_MAJOR)
cmor.dataset_json("Test/common_user_input.json")

tables = []
tables.append(cmor.load_table("mytable"))
print 'Tables ids:', tables


# read in data, just one slice
f = cdms2.open('data/tas_ccsr-95a.xml')
s = f("tas", time=slice(0, 12), squeeze=1)
ntimes = 12
varout = 'tas'

myaxes = numpy.arange(10)
myvars = numpy.arange(10)
myaxes[0] = cmor.axis(table_entry='latitude',
                      units='degrees_north',
                      coord_vals=s.getLatitude()[:], cell_bounds=s.getLatitude().getBounds())
myaxes[1] = cmor.axis(table_entry='longitude',
                      units='degrees_north',
                      coord_vals=s.getLongitude()[:], cell_bounds=s.getLongitude().getBounds())


myaxes[2] = cmor.axis(table_entry='time',
                      units=s.getTime().units,
                      coord_vals=s.getTime()[:], cell_bounds=s.getTime().getBounds())

pass_axes = [myaxes[2], myaxes[0], myaxes[1]]

myvars[0] = cmor.variable(table_entry=varout,
                          units=s.units,
                          axis_ids=pass_axes,
                          original_name=s.id,
                          history='no history',
                          comment='testing speed'
                          )


import time
import MV2
st = time.time()
totcmor = 0
totcdms = 0
maxcmor = 0
mincmor = 1000
maxcdms = 0
mincdms = 1000
c0 = st
f = cdms2.open("Test/crap.nc", "w")
# print 'Time:',i
print s.filled().shape
cmor.write(myvars[0], s.filled(), ntimes_passed=ntimes)
c = time.time()
print 'cmor write time:', c - c0
totcmor += c - c0
if maxcmor < c - c0:
    maxcmor = c - c0
if mincmor > c - c0:
    mincmor = c - c0
c0 = c
f.write(s, id=varout)
c = time.time()
print 'cdms time:', c - c0
totcdms += c - c0
if maxcdms < c - c0:
    maxcdms = c - c0
if mincdms > c - c0:
    mincdms = c - c0
c0 = c
f.close()
cmor.close()

import cdtime
import os
ltime = cdtime.reltime(ntimes - 1, 'month since 1980').tocomp()
#lcmor = os.stat("CMIP6/CMIP/CSIRO-BOM/NICAM/piControl/r1i1p1f1/Amon/tas/gn/v%s/tas_Amon_piControl_NICAM_r1i1p1f1_gn_197901-197912.nc"%(today))[6]
lcmor = os.stat(
    "CMIP6/ISMIP6/PCMDI/PCMDI-test-1-0/piControl-withism/r11i1p1f1/Amon/tas/gr/v%s/tas_Amon_piControl-withism_PCMDI-test-1-0_r11i1p1f1_gr_197901-197912.nc" %
    (today))[6]
print 'level:', level, "shuffle:", shuffle
print 'total cmor:', totcmor, mincmor, totcmor / ntimes, maxcmor, lcmor
lcdms = os.stat("Test/crap.nc")[6]
print 'total cdms:', totcdms, mincdms, totcdms / ntimes, maxcdms, lcdms
print 'Size diff:', float(lcmor) / float(lcdms)
print 'speed diff:', totcmor / totcdms

if os.path.exists("summary.txt"):
    f = open("summary.txt")
    s = f.read()
    f.close()
    dic = eval(s)
else:
    dic = {}

dic[(level, shuffle)] = (float(lcmor) / float(lcdms), totcmor / totcdms)

for i in range(10):
    a = dic.get((i, 0), "N/A")
    b = dic.get((i, 1), "N/A")
    print 'Level: ', i, "no suffle:", a, "shuffle", b
f = open("summary.txt", "w")
f.write(repr(dic))
f.close()
