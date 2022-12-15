from __future__ import print_function
from test_python_common import *  # common subroutines

import cmor._cmor
import os

pth = os.path.split(os.path.realpath(os.curdir))


myaxes = numpy.zeros(9, dtype='i')
myaxes2 = numpy.zeros(9, dtype='i')
myvars = numpy.zeros(9, dtype='i')


cmor.setup(
    inpath="Tables",
    set_verbosity=cmor.CMOR_NORMAL,
    netcdf_file_action=cmor.CMOR_REPLACE,
    exit_control=cmor.CMOR_EXIT_ON_MAJOR)
cmor.dataset_json("Test/CMOR_input_example.json")

tables = []
a = cmor.load_table("Tables/CMIP6_grids.json")
tables.append(a)
tables.append(cmor.load_table("Tables/CMIP6_Omon.json"))
print('Tables ids:', tables)

cmor.set_table(tables[0])

x, y, lon_coords, lat_coords, lon_vertices, lat_vertices = gen_irreg_grid(
    lon, lat)


myaxes[0] = cmor.axis(table_entry='y',
                      units='m',
                      coord_vals=y)
myaxes[1] = cmor.axis(table_entry='x',
                      units='m',
                      coord_vals=x)

grid_id = cmor.grid(axis_ids=myaxes[:2],
                    latitude=lat_coords,
                    longitude=lon_coords,
                    latitude_vertices=lat_vertices,
                    longitude_vertices=lon_vertices)
print('got grid_id:', grid_id)
myaxes[2] = grid_id

## mapnm = 'lambert_conformal_conic'
# params = [ "standard_parallel1",
# "longitude_of_central_meridian","latitude_of_projection_origin",
# "false_easting","false_northing","standard_parallel2" ]
## punits = ["","","","","","" ]
## pvalues = [-20.,175.,13.,8.,0.,20. ]
# cmor.set_grid_mapping(grid_id=myaxes[2],
##                       mapping_name = mapnm,
##                       parameter_names = params,
##                       parameter_values = pvalues,
# parameter_units = punits)

cmor.set_table(tables[1])
myaxes[3] = cmor.axis(table_entry='time',
                      units='months since 1980')
myaxes[4] = cmor.axis(table_entry='oline',
                      units='',
                      coord_vals="""barents_opening bering_strait canadian_archipelago denmark_strait drake_passage english_channel pacific_equatorial_undercurrent faroe_scotland_channel florida_bahamas_strait fram_strait iceland_faroe_channel indonesian_thoughflow mozambique_channel taiwan_luzon_straits windward_passage""".split())

pass_axes = [myaxes[3], myaxes[4]]

print('ok going to cmorvar')
myvars[0] = cmor.variable(table_entry='mfo',
                          units='kg s-1',
                          axis_ids=pass_axes,
                          history='no history',
                          comment='no future'
                          )
for i in range(ntimes):
    data2d = numpy.random.random((1, 15))
    print('writing time: ', i, data2d.shape, data2d)
    print(Time[i], bnds_time[2 * i:2 * i + 2])
    cmor.write(myvars[0], data2d, 1, time_vals=Time[i],
               time_bnds=bnds_time[2 * i:2 * i + 2])
cmor.close()
