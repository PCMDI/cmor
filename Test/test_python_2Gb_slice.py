from __future__ import print_function
import cmor
import numpy


nlat = 3600
dlat = 180. / nlat
nlon = 7200
dlon = 360. / nlon
nlev = 26
dlev = 1000. / nlev
ntimes = 1

lats = numpy.arange(-90 + dlat / 2., 90, dlat)
blats = numpy.arange(-90, 90 + dlat, dlat)
lons = numpy.arange(0 + dlon / 2., 360., dlon)
blons = numpy.arange(0, 360. + dlon, dlon)

levs = numpy.array([1000., 925, 900, 850, 800, 700, 600, 500, 400, 300,
                    250, 200, 150, 100, 75, 70, 50, 30, 20, 10, 7.5, 5, 2.5, 1, .5, .1])
alllevs = numpy.arange(1000, 0, -dlev).tolist()
print(len(alllevs))

cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
cmor.dataset_json("Test/CMOR_input_example.json")
table = 'CMIP6_Amon.json'
cmor.load_table(table)


ilat = cmor.axis(
    table_entry='latitude',
    coord_vals=lats,
    cell_bounds=blats,
    units='degrees_north')
ilon = cmor.axis(
    table_entry='longitude',
    coord_vals=lons,
    cell_bounds=blons,
    units='degrees_east')
# ,coord_vals=numpy.arange(ntimes,dtype=float),cell_bounds=numpy.arange(ntimes+1,dtype=float),units='months since 2000')
itim = cmor.axis(table_entry='time', units='months since 2010')
ilev = cmor.axis(table_entry='plev19', coord_vals=levs, units='hPa')

axes = [itim, ilev, ilat, ilon]

var = cmor.variable(table_entry='ta', units='K', axis_ids=axes)

print("allocating mem for data")
data = numpy.random.random((nlev, nlat, nlon)) * 30 + 273.15
print("moving on to writing")

for i in range(ntimes):
    print('Writing time:', i)
    cmor.write(var, data, time_vals=numpy.array(
        [float(i), ]), time_bnds=numpy.array([i, i + 1.]))

print("closing var")
print(cmor.close(var_id=var, file_name=True))
print("closing cmor")
cmor.close()
print("done")


print('hello')
