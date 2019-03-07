import cdms2
import cmor
import numpy
import os

specs = {"CLOUD": {"convert": [2.0, 0.], "units": "%", "entry": "cl", "positive": ""},
         "U": {"convert": [1., -40.], "units": "m s-1", "entry": "ua", "positive": ""},
         "T": {"convert": [1., 220], "units": "K", "entry": "ta", "positive": ""},
         "LATENT": {"convert": [4., 0.], "units": "W m-2", "entry": "hfls", "positive": "down"},
         "TSURF": {"convert": [1., -15.], "units": "Celsius", "entry": "tas", "positive": ""},
         "PSURF": {"convert": [3., 900.], "units": "hPa", "entry": "ps", "positive": ""},
         "SOIL_WET": {"convert": [2., 0.], "units": "kg m-2", "entry": "mrsos", "positive": ""},
        "MC": {"convert": [2., 0.], "units": "kg m-2 s-1", "entry": "mc", "positive": "up"},
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

    print("3d shape",shape)
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


varin3d = ["MC",]

n3d = len(varin3d)

alats, bnds_lat, alons, bnds_lon, plevs = read_coords(lat, lon)

print(alats[:2], alats[-2:])
print(bnds_lat[0], bnds_lat[-1])
print(alons[:2], alons[-2:])
print(bnds_lon[0], bnds_lon[-1])
print(plevs)

pth = os.path.expanduser("~/Karl")
pth = os.getcwd()
ierr = cmor.setup(inpath=os.path.join(pth, "Test"),
                  netcdf_file_action=cmor.CMOR_REPLACE) #, logfile="CMOR.log")
print("ERR:", ierr)
ierr = cmor.dataset_json(os.path.join(pth, "Test", "CMOR_input_example.json"))
print("ERR:", ierr)

ierr = cmor.load_table(os.path.join(pth, 'Tables', 'CMIP6_Amon.json'))
print("ERR:", ierr)
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

print("ITIME:",itim)
zlevs = numpy.array([.1, .3, .55, .7, .9])
zlev_bnds = numpy.array([0., .2, .42, .62, .8, 1.])

ilev_half = cmor.axis(table_entry='standard_hybrid_sigma_half',
                 units='1',
                 coord_vals=zlevs, cell_bounds=zlev_bnds)
print("ILEVL half:",ilev_half)

cmor.zfactor(zaxis_id=ilev_half, zfactor_name='p0', units='hPa', zfactor_values=p0)
print("p0 1/2")
cmor.zfactor(zaxis_id=ilev_half, zfactor_name='b_half', axis_ids=[ilev_half, ],
             zfactor_values=b_coeff, zfactor_bounds=b_coeff_bnds)
print("b 1/2")
cmor.zfactor(zaxis_id=ilev_half, zfactor_name='a_half', axis_ids=[ilev_half, ],
             zfactor_values=a_coeff, zfactor_bounds=a_coeff_bnds)
print("a 1/2")
ps_var = cmor.zfactor(zaxis_id=ilev_half, zfactor_name='ps',
             axis_ids=[ilon, ilat, itim], units='hPa')
print("ps 1/2")

var3d_ids = []
for m in varin3d:
    print("3d VAR:",m)
    var3d_ids.append(
        cmor.variable(table_entry=specs[m]["entry"],
                      units=specs[m]["units"],
                      axis_ids=[itim, ilev_half, ilat, ilon],
                      missing_value=1.e28,
                      positive=specs[m]["positive"],
                      original_name=m)
    )
print("Ok now writing",var3d_ids, ntimes)
for index in range(ntimes):
    tim_array, bnds_tim = read_time(index)
    for i, varname in enumerate(varin3d):
        data = read_3d_input_files(index, varname,(lev,lat,lon))
        print(data.shape, data)
        print(tim_array, bnds_tim)
        cmor.write(var_id=var3d_ids[i], data=data, ntimes_passed=1,
                   time_vals=tim_array, time_bnds=bnds_tim)
        print("Passed write")
        # PSURF
        data = read_2d_input_files(index,"PSURF",(lat,lon))
        cmor.write(var_id=ps_var, data=data, ntimes_passed=1, time_vals=tim_array, time_bnds=bnds_tim, store_with=var3d_ids[i])
cmor.close()