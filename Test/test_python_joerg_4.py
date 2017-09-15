import cmor
import numpy

error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)

error_flag = cmor.dataset_json("Test/common_user_input.json")

# creates 1 degree grid
nlat = 18
nlon = 36
alats = numpy.arange(180) - 89.5
bnds_lat = numpy.arange(181) - 90
alons = numpy.arange(360) + .5
bnds_lon = numpy.arange(361)
cmor.load_table("Tables/CMIP6_Omon.json")
ilat = cmor.axis(
    table_entry='latitude',
    units='degrees_north',
    length=nlat,
    coord_vals=alats,
    cell_bounds=bnds_lat)

ilon = cmor.axis(
    table_entry='longitude',
    length=nlon,
    units='degrees_east',
    coord_vals=alons,
    cell_bounds=bnds_lon)

ntimes = 12
plevs = sorted(numpy.array([0, 17.0, 27.0, 37.0, 47.0, 57.0, 68.0, 82.0]))
plevs_bnds = sorted(numpy.array(
    [0, 11, 22.0, 32.0, 42.0, 52.0, 62.5, 75.0, 91.0]))

itim = cmor.axis(
    table_entry='time',
    units='months since 2030-1-1',
    length=ntimes,
    interval='1 month')

ilev = cmor.axis(
    table_entry='depth0m',
    units='m',
    coord_vals=plevs,
    cell_bounds=plevs_bnds)
try:
    ilev = cmor.axis(
        table_entry='depth0m',
        units='m',
        coord_vals=plevs,
        cell_bounds=plevs_bnds)
except BaseException:
    pass

var3d_ids = cmor.variable(
    table_entry='co3',
    units='mol m-3',
    axis_ids=numpy.array([ilon, ilat, itim]),
    missing_value=numpy.array([1.0e28, ], dtype=numpy.float32)[0],
    original_name='cloud')


for it in range(ntimes):

    time = numpy.array((it))
    bnds_time = numpy.array((it, it + 1))
    data3d = numpy.random.random((nlon, nlat)) * 30. + 265.
    data3d = data3d.astype('f')
    error_flag = cmor.write(
        var_id=var3d_ids,
        data=data3d,
        ntimes_passed=1,
        time_vals=time,
        time_bnds=bnds_time)


error_flag = cmor.close()
