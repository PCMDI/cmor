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


cmor.setup(
    inpath="Tables",
    set_verbosity=cmor.CMOR_NORMAL,
    netcdf_file_action=cmor.CMOR_REPLACE_4,
    exit_control=cmor.CMOR_EXIT_ON_MAJOR)
cmor.dataset_json("Test/common_user_input.json")

tables = []
tables.append(cmor.load_table("CMIP6_chunking.json"))
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


cmor.write(myvars[0], s.filled(), ntimes_passed=ntimes)
cmor.close()

#lcmor = os.stat("CMIP6/ISMIP6/PCMDI/PCMDI-test-1-0/piControl-withism/r11i1p1f1/Amon/tas/gr/v%s/tas_Amon_piControl-withism_PCMDI-test-1-0_r11i1p1f1_gr_197901-197912.nc"%(today))[6]
