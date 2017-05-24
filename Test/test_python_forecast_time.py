from test_python_common import *  # common subroutines

import cmor._cmor
import os

dtmp2 = 1.e-4
pth = os.path.split(os.path.realpath(os.curdir))
if pth[-1] == 'Test':
    ipth = opth = '.'
else:
    ipth = opth = 'Test'

myaxes = numpy.zeros(9, dtype='i')
myaxes2 = numpy.zeros(9, dtype='i')
myvars = numpy.zeros(9, dtype='i')

cmor.setup(
    inpath="Tables",
    set_verbosity=cmor.CMOR_NORMAL,
    netcdf_file_action=cmor.CMOR_REPLACE,
    exit_control=cmor.CMOR_EXIT_ON_MAJOR)

cmor.dataset_json("Test/common_user_input.json")

table = cmor.load_table(os.path.join("CMIP6_Amon.json"))
print 'Tables ids:', table

axes = []
# ok we need to make the bounds 2D because the cmor module "undoes this"
bnds_time = numpy.reshape(bnds_time, (bnds_time.shape[0] / 2, 2))
bnds_lat = numpy.reshape(bnds_lat, (bnds_lat.shape[0] / 2, 2))
bnds_lon = numpy.reshape(bnds_lon, (bnds_lon.shape[0] / 2, 2))

id = 'time'
units = 'months since 1980'
myaxes[0] = cmor.axis(
    id,
    coord_vals=Time,
    units=units,
    cell_bounds=bnds_time,
    interval="1 month")

id = 'latitude'
units = "degrees_north"
myaxes[1] = cmor.axis(id, coord_vals=alats, units=units, cell_bounds=bnds_lat)

id = "longitude"
units = "degrees_east"
myaxes[2] = cmor.axis(id, coord_vals=alons, units=units, cell_bounds=bnds_lon)


myaxes[3] = cmor.axis(
    "forecast_time",
    coord_vals=[20],
    units="days since 1979-01-01")

#myaxes[3] = cmor.axis(
#    "forecast_time",
#    coord_vals=[0],
#    units="m")

print 'ok doing the vars thing', positive2d[0]
myvars[0] = cmor.variable(entry2d[0],
                          units2d[0],
                          myaxes[:4],
                          'd',
                          missing_value=None,
                          tolerance=dtmp2,
                          positive=positive2d[0],
                          original_name=varin2d[0],
                          history="no history",
                          comment="no future")

#  /* ok here we decalre a variable for region axis testing */

for i in range(ntimes):
    data2d = read_2d_input_files(i, varin2d[0], lat, lon)
    print 'writing time: ', i, data2d.shape, data2d
    cmor.write(myvars[0], data2d, 1)

cmor.close()
