import cmor

import numpy

cmor.setup("Test", netcdf_file_action=cmor.CMOR_REPLACE)

cmor.dataset_json("Test/CMOR_input_example.json")

cmor.load_table("Tables/CMIP6_Omon.json")

nlat = 90
dlat = 180 / nlat
lats = numpy.arange(-90 + dlat / 2., 90, dlat)
blats = numpy.arange(-90, 90 + dlat, dlat)

ilat = cmor.axis(
    table_entry='latitude',
    coord_vals=lats,
    cell_bounds=blats,
    units='degrees_north')

myregions = [
    "atlantic_ocean",
    "indian_pacific_ocean",
    "pacific_ocean",
    "atlantic_arctic_ocean",
    "global_ocean"]
nreg = len(myregions)
ireg = cmor.axis(table_entry='basin', units='1', coord_vals=myregions)
ntime = 12

itim = cmor.axis(
    table_entry='time',
    units='months since 2030',
    interval='1 month',
    coord_vals=numpy.arange(ntime),
    cell_bounds=numpy.arange(
        ntime + 1))


var = cmor.variable(table_entry='htovgyre', units='W',
                    axis_ids=numpy.array([itim, ireg, ilat]))

data = numpy.random.random((ntime, nreg, nlat)) * 3.E14

cmor.write(var, data)
cmor.close()
