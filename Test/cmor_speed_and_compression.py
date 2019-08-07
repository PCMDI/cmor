from __future__ import print_function
import cmor
import numpy
import tempfile
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
    print("This test code needs a recent cdms2 interface for i/0")
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
f = tempfile.NamedTemporaryFile(mode='w', prefix='cmor_speed_and_compression', dir='.')
f.write(s)
mytable = os.path.basename(f.name)

cmor.setup(
    inpath="Tables",
    set_verbosity=cmor.CMOR_NORMAL,
    netcdf_file_action=cmor.CMOR_REPLACE_4,
    exit_control=cmor.CMOR_EXIT_ON_MAJOR)
cmor.dataset_json("Test/CMOR_input_example.json")

tables = []
tables.append(cmor.load_table(mytable))
print('Tables ids:', tables)
f.close()


# read in data, just one slice
f = cdms2.open('data/tas_ccsr-95a.xml')


s = f("tas", time=slice(0, 1), squeeze=1)

ntimes = 12

myaxes = numpy.arange(10)
myvars = numpy.arange(10)
myaxes[0] = cmor.axis(table_entry='latitude',
                      units='degrees_north',
                      coord_vals=s.getLatitude()[:], cell_bounds=s.getLatitude().getBounds())
myaxes[1] = cmor.axis(table_entry='longitude',
                      units='degrees_north',
                      coord_vals=s.getLongitude()[:], cell_bounds=s.getLongitude().getBounds())


myaxes[3] = cmor.axis(table_entry='time',
                      units='months since 1980',
                      coord_vals=numpy.arange(ntimes), cell_bounds=numpy.arange(ntimes + 1))

pass_axes = [myaxes[2], myaxes[0], myaxes[1]]

myvars[0] = cmor.variable(table_entry='tas',
                          units='K',
                          axis_ids=pass_axes,
                          original_name='CLT',
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
s2 = s * 1
sh = list(s.shape)
print('shape:', sh)
sh.insert(0, 1)
s2 = MV2.reshape(s2, sh)
s2.setAxis(1, s.getLatitude())
s2.setAxis(2, s.getLongitude())
f = cdms2.open("Test/crap.nc", "w")
for i in range(ntimes):
    # print 'Time:',i
    cmor.write(myvars[0], s.filled(), 1)
    c = time.time()
    # print 'cmor write time:',c-c0
    totcmor += c - c0
    if maxcmor < c - c0:
        maxcmor = c - c0
    if mincmor > c - c0:
        mincmor = c - c0
    c0 = c
    t = cdms2.createAxis([i])
    t.id = 'time'
    t.units = 'months since 1980'
    t.designateTime()
    s2.setAxis(0, t)
    f.write(s2, id='tas')
    c = time.time()
    # print 'cdms time:',c-c0
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
#lcmor = os.stat("CMIP6/CMIP/CSIRO-BOM/NICAM/piControl/r1i1p1f1/Amon/tas/gn/v%s/tas_Amon_piControl_NICAM_r1i1p1f1_gn_198001-%i%.2i.nc" % (today,ltime.year,ltime.month))[6]
lcmor = os.stat(
    "CMIP6/CMIP6/ISMIP6/PCMDI/PCMDI-test-1-0/piControl-withism/r3i1p1f1/Amon/tas/gn/v%s/tas_Amon_PCMDI-test-1-0_piControl-withism_r3i1p1f1_gn_198001-%i%.2i.nc" %
    (today, ltime.year, ltime.month))[6]
print('level:', level, "shuffle:", shuffle)
print('total cmor:', totcmor, mincmor, totcmor / ntimes, maxcmor, lcmor)
lcdms = os.stat("Test/crap.nc")[6]
print('total cdms:', totcdms, mincdms, totcdms / ntimes, maxcdms, lcdms)
print('Size diff:', float(lcmor) / float(lcdms))
print('speed diff:', totcmor / totcdms)

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
    print('Level: ', i, "no suffle:", a, "shuffle", b)
f = open("summary.txt", "w")
f.write(repr(dic))
f.close()
