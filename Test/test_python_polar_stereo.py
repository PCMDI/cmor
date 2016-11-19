#!/usr/bin/env python

import cmor
import cdms2
import numpy
import os

# Create some empty arrays
# -------------------------
myaxes=numpy.zeros(9,dtype='i')
myaxes2=numpy.zeros(9,dtype='i')
myvars=numpy.zeros(9,dtype='i')

# Initialize CMOR
# -------------------
cmor.setup(inpath="Tables",set_verbosity=cmor.CMOR_NORMAL, netcdf_file_action = cmor.CMOR_REPLACE_4 );
cmor.dataset_json("Test/common_user_input.json")

tables=[]
grid_table = cmor.load_table("CMIP6_grids.json")
tables.append(grid_table)

t='CMIP6_fx.json'
te = 'orog'
u='m'

tables.append(cmor.load_table("%s" % t))
print 'Tables ids:',tables

cmor.set_table(tables[0])

f=cdms2.open("~/Downloads/orog_GIS_LGGE_ELMER2_asmb.nc")
fg=cdms2.open("~/Downloads/Greenland_5km_v1.1.nc")
x=f['x'][:]
y=f['y'][:]
lon_coords=fg['lon'][0,:]
lat_coords=fg['lat'][0,:]



myaxes[0] = cmor.axis(table_entry = 'y', 
                      units = 'm', 
                      coord_vals = y)
myaxes[1] = cmor.axis(table_entry = 'x', 
                      units = 'm', 
                      coord_vals = x)

grid_id = cmor.grid(axis_ids = myaxes[:2], 
                    latitude = lat_coords, 
                    longitude = lon_coords) 

print 'got grid_id:',grid_id
myaxes[2] = grid_id

mapnm = 'polar_stereographic'
params = [ "standard_parallel",
           "latitude_of_projection_origin",
           "false_easting",
           "false_northing",
           "straight_vertical_longitude_from_pole",
           "scale_factor_at_projection_origin"]

punits = ["","","","","","" ]
pvalues = [71.,90.,0.,0.,-39., 1. ]
cmor.set_grid_mapping(grid_id=myaxes[2],
                      mapping_name = mapnm,
                      parameter_names = params,
                      parameter_values = pvalues,
                      parameter_units = punits)

cmor.set_table(tables[1])
pass_axes = [myaxes[2]]

data = f['orog'][0,:]
myvars[0] = cmor.variable( table_entry = te,
                           units = u,
                           axis_ids = pass_axes,
                           missing_value = data.missing,
                           history = '',
                           comment = ''
                           )
cmor.write(myvars[0], data)

cmor.close()
