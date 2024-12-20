from collections import namedtuple

import cmor
import numpy


Grid = namedtuple('Grid', ['x', 'y',
                           'x_bnds', 'y_bnds',
                           'lon', 'lat',
                           'lon_bnds', 'lat_bnds'])


def gen_time(ntimes):
    time_vals = numpy.arange(ntimes)
    time_bnds = [[t, t + 1] for t in time_vals]
    return time_vals, time_bnds


def gen_irreg_grid(nlon, nlat):
    lon0 = 5.
    lat0 = -17.5
    delta_lon = .1
    delta_lat = .1
    y = numpy.arange(nlat) + 1.
    x = numpy.arange(nlon) + 1.
    x_bnds = [[_x - 0.5, _x + 0.5] for _x in x]
    y_bnds = [[_y - 0.5, _y + 0.5] for _y in y]
    lon = numpy.zeros((nlat, nlon))
    lat = numpy.zeros((nlat, nlon))
    lon_bnds = numpy.zeros((nlat, nlon, 4))
    lat_bnds = numpy.zeros((nlat, nlon, 4))

    for j in range(nlat):
        for i in range(nlon):
            lon[j, i] = lon0 + delta_lon * (j + 1 + i)
            lat[j, i] = lat0 + delta_lat * (j + 1 - i)
            lon_bnds[j, i, 0] = lon[j, i] - delta_lon
            lon_bnds[j, i, 1] = lon[j, i]
            lon_bnds[j, i, 2] = lon[j, i] + delta_lon
            lon_bnds[j, i, 3] = lon[j, i]
            lat_bnds[j, i, 0] = lat[j, i]
            lat_bnds[j, i, 1] = lat[j, i] - delta_lat
            lat_bnds[j, i, 2] = lat[j, i]
            lat_bnds[j, i, 3] = lat[j, i] + delta_lat
    return Grid(x, y, x_bnds, y_bnds, lon, lat, lon_bnds, lat_bnds)


def main():

    ntimes = 2
    nlon = 300
    nlat = 100

    cmor.setup(
        inpath='Tables',
        set_verbosity=cmor.CMOR_NORMAL,
        netcdf_file_action=cmor.CMOR_REPLACE,
        exit_control=cmor.CMOR_EXIT_ON_MAJOR)
    cmor.dataset_json('Test/CMOR_input_example.json')

    grid_table_id = cmor.load_table('CMIP6_grids.json')
    cmor.set_table(grid_table_id)

    grid = gen_irreg_grid(nlon, nlat)

    y_axis_id = cmor.axis(table_entry='y_deg', units='degrees',
                          coord_vals=grid.y, cell_bounds=grid.y_bnds)
    x_axis_id = cmor.axis(table_entry='x_deg', units='degrees',
                          coord_vals=grid.x, cell_bounds=grid.x_bnds)

    grid_id = cmor.grid(axis_ids=[y_axis_id, x_axis_id],
                        latitude=grid.lat,
                        longitude=grid.lon,
                        latitude_vertices=grid.lat_bnds,
                        longitude_vertices=grid.lon_bnds)

    omon_table_id = cmor.load_table('CMIP6_Omon.json')
    cmor.set_table(omon_table_id)

    time_vals, time_bnds = gen_time(ntimes)
    time_axis_id = cmor.axis(table_entry='time', units='months since 1980',
                             coord_vals=time_vals, cell_bounds=time_bnds)

    var_id = cmor.variable(table_entry='sos', units='0.001',
                           axis_ids=[grid_id, time_axis_id])

    var_data = numpy.random.random((nlat, nlon, ntimes)) * 40. + 273.15
    cmor.write(var_id, var_data, ntimes)

    cmor.close()


if __name__ == '__main__':
    main()
