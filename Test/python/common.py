from __future__ import print_function
import numpy
import cmor
import os

specs = {"CLOUD": {"convert": [2.0, 0.], "units": "%", "entry": "cl", "positive": ""},
         "U": {"convert": [1., -40.], "units": "m s-1", "entry": "ua", "positive": ""},
         "T": {"convert": [1., 220], "units": "K", "entry": "ta", "positive": ""},
         "LATENT": {"convert": [4., 0.], "units": "W m-2", "entry": "hfls", "positive": "down"},
         "TSURF": {"convert": [1., -15.], "units": "Celsius", "entry": "tas", "positive": ""},
         "PSURF": {"convert": [3., 900.], "units": "hPa", "entry": "ps", "positive": ""},
         "SOIL_WET": {"convert": [2., 0.], "units": "kg m-2", "entry": "mrsos", "positive": ""},
         "MC": {"convert": [2., 0.], "units": "kg m-2 s-1", "entry": "mc", "positive": "up"},
         "BS": {"convert": [2., 0.], "units": "m-1 sr-1", "entry": "bs550aer", "positive": ""},
         }


def read_coords(nlats, nlons):
    alons = (360.*numpy.arange(nlons)/nlons+.5) - 180.
    blons = numpy.zeros((nlons, 2), dtype=float)
    blons[:, 0] = 360.*numpy.arange(nlons)/nlons - 180.
    blons[:, 1] = 360.*numpy.arange(1, nlons+1)/nlons - 180.

    alats = (180.*numpy.arange(nlats)/nlats+.5) - 90.
    blats = numpy.zeros((nlats, 2), dtype=float)
    blats[:, 0] = 180.*numpy.arange(nlats)/nlats - 90.
    blats[:, 1] = 180.*numpy.arange(1, nlats+1)/nlats - 90.

    levs = numpy.array([1., 5., 10, 20, 30, 50, 70, 100, 150,
                        200, 250, 300, 400, 500, 600, 700, 850, 925, 1000])

    return alats, blats, alons, blons, levs


def read_time(index, refyear=2015., monthdays=30., yeardays=360.):
    time0 = (refyear - 1850.) * yeardays
    time = time0 + (index + 0.5) * monthdays
    b1 = time0 + (index) * monthdays
    b2 = time0 + (index+1) * monthdays
    return numpy.array([time]), numpy.array([b1, b2])


def read_3d_input_files(index, varname, shape):

    print("3d shape", shape)
    field = numpy.zeros(shape, dtype=numpy.float32)
    factor, offset = specs[varname]["convert"]
    for i in range(field.shape[2]):  # lon
        for j in range(field.shape[1]):  # lat
            for k in range(field.shape[0]):  # levs
                if i == 0:
                    field[k, j, 0] = ((k)*3 + (j)*16 + index) * \
                        factor + offset + 0.225
                else:
                    field[k, j, i] = ((k)*3 + (j)*16 + index +
                                      4*i-2)*factor + offset + 0.225
    return field


def read_2d_input_files(index, varname, shape):
    sh = list(shape)
    sh.insert(0, 1)
    return read_3d_input_files(index, varname, sh)[0]


# PARAMETERS

ntimes = 2
lon = 4
lat = 3
plev = 19
lev = 5

p0 = 1.e3
a_coeff = [0.1, 0.2, 0.3, 0.22, 0.1]
b_coeff = [0.0, 0.1, 0.2, 0.5, 0.8]

a_coeff_bnds = [0., .15, .25, .25, .16, 0.]
b_coeff_bnds = [0., .05, .15, .35, .65, 1.]


zlevs = numpy.array([.1, .3, .55, .7, .9])
zlev_bnds = numpy.array([0., .2, .42, .62, .8, 1.])

alats, bnds_lat, alons, bnds_lon, plevs = read_coords(lat, lon)

def init_cmor(pth, dataset_json):
    ierr = cmor.setup(inpath=os.path.join(pth, "Test"),
                    netcdf_file_action=cmor.CMOR_REPLACE) #, logfile="CMOR.log")
    ierr = cmor.dataset_json(os.path.join(pth, "Test", dataset_json))

def read_cmor_time_lat_lon():
    ilat = cmor.axis(
        table_entry='latitude',
        units='degrees_north',
        length=lat,
        coord_vals=alats,
        cell_bounds=bnds_lat)
    print("ILAT:", ilat)
    print(lon, alons, bnds_lon)
    ilon = cmor.axis(
        table_entry='longitude',
        coord_vals=alons,
        units='degrees_east',
        cell_bounds=bnds_lon)

    print("ILON:", ilon)


    itim = cmor.axis(
        table_entry="time",
        units="days since 1850",
        length=ntimes)
    return itim, ilat, ilon

def read_cmor_time1_lat_lon():
    ilat = cmor.axis(
        table_entry='latitude',
        units='degrees_north',
        length=lat,
        coord_vals=alats,
        cell_bounds=bnds_lat)
    print("ILAT:", ilat)
    print(lon, alons, bnds_lon)
    ilon = cmor.axis(
        table_entry='longitude',
        coord_vals=alons,
        units='degrees_east',
        cell_bounds=bnds_lon)

    print("ILON:", ilon)


    itim = cmor.axis(
        table_entry="time1",
        units="days since 1850",
        length=ntimes)
    return itim, ilat, ilon