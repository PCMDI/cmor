
import cmor
import numpy


nlat = 360
dlat = 180. / nlat
nlon = 720
dlon = 360. / nlon
nlev = 19
ntimes = 12

lats = numpy.arange(-90 + dlat / 2., 90, dlat)
blats = numpy.arange(-90, 90 + dlat, dlat)
lons = numpy.arange(0 + dlon / 2., 360., dlon)
blons = numpy.arange(0, 360. + dlon, dlon)


cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
cmor.dataset_json("Test/common_user_input.json")
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
# ,coord_vals=numpy.arange(ntimes,dtype=numpy.float),cell_bounds=numpy.arange(ntimes+1,dtype=float),units='months since 2000')
itim = cmor.axis(table_entry='time', units='months since 2010')
ilev = cmor.axis(table_entry='plev19',
                 coord_vals=numpy.array([1000.,
                                         925,
                                         850,
                                         700,
                                         600,
                                         500,
                                         400,
                                         300,
                                         250,
                                         200,
                                         150,
                                         100,
                                         70,
                                         50,
                                         30,
                                         20,
                                         10,
                                         5,
                                         1]),
                 units='hPa')

axes = [itim, ilev, ilat, ilon]

var = cmor.variable(table_entry='ta', units='K', axis_ids=axes)
ntimes = 250

data = numpy.random.random((nlev, nlat, nlon)) * 30 + 273.15

for i in range(ntimes):
    if i % 10 == 0:
        print 'Writing time:', i
    cmor.write(var, data, time_vals=numpy.array(
        [float(i), ]), time_bnds=numpy.array([i, i + 1.]))

print cmor.close(var_id=var, file_name=True)
cmor.close()


print 'hello'
