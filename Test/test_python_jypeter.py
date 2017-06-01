#!/usr/bin/env python

# script used to check if time dimensions was passed to CMOR_Write.
# This resolves issue_20 and is not part of all make test_python
#
import numpy, cdms2, cmor

# Generate a dummy data matrix with '32 degF' everywhere except 2
# latitudinal bands around the equator
raw_data = numpy.ma.ones((1, 10, 18), numpy.float32) * 32
raw_data[:, 4:6, :] = numpy.ma.masked

# Generate axes for the data matrix
lat_axis = cdms2.createUniformLatitudeAxis(90, 10, -20)
lon_axis = cdms2.createUniformLongitudeAxis(-180, 18, 20)

cmor.setup(inpath='./Tables',
           netcdf_file_action=cmor.CMOR_REPLACE)

cmor.dataset_json("Test/test_python_jypeter.json")
cmor.load_table('CMIP6_Amon.json')

time_id = cmor.axis(table_entry= 'time',
                    units= 'days since 2000-01-01 00:00:00')

lat_id = cmor.axis(table_entry= 'latitude',
                   units= 'degrees_north',
                   coord_vals=lat_axis[:],
                   cell_bounds=lat_axis.getBounds())
lon_id = cmor.axis(table_entry= 'longitude',
                   units= 'degrees_east',
                   coord_vals=lon_axis[:],
                   cell_bounds=lon_axis.getBounds())

data_id = cmor.variable(table_entry='ts',
                        units='degF',
                        axis_ids=[time_id, lat_id, lon_id],
                        missing_value=raw_data.fill_value,
                        original_name='temp',
                        comment='A dummy variable from a dummy dataset')

# Write 3 time steps to the file (January to March). Add 18 degF to
# each time step (which should be converted to 10 K in the output
# file)
for month in range(3):
    cmor.write(var_id=data_id,
               data=raw_data + month * 18,
# Uncomment the 2 lines below in order to have a working script
#               time_vals=numpy.array([15. + 30 * month]),
#               time_bnds=numpy.array([0. + 30 * month, 30. + 30 * month])
)

file_path = cmor.close(data_id, file_name=True)

print '\nThe dummy data was successfully saved to', file_path

# The end
