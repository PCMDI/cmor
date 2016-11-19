
import cmor,numpy
import os
ntimes=2
lon=300
lat=100
lev=5

def read_time(it):
    time = [0]
    time_bnds=[0,0]
    time[0] = (it-0.5)*30.;
    time_bnds[0] = (it-1)*30.;
    time_bnds[1] = it*30.;

    time[0]=it;
    time_bnds[0] = it;
    time_bnds[1] = it+1;
    return time[0],numpy.array(time_bnds)

def gen_irreg_grid(lon,lat):
    lon0 = 5.
    lat0=-17.5
    delta_lon = .1
    delta_lat = .1
    y = numpy.arange(lat)
    x = numpy.arange(lon)
    lon_coords = numpy.zeros((lat,lon))
    lat_coords = numpy.zeros((lat,lon))
    lon_vertices = numpy.zeros((lat,lon,4))
    lat_vertices = numpy.zeros((lat,lon,4))

    for j in range(lat): # really porr coding i know
        for i in range(lon): # getting worse i know
            lon_coords[j,i] = lon0+delta_lon*(j+1+i);
            lat_coords[j,i] = lat0+delta_lat*(j+1-i);
            lon_vertices[j,i,0] = lon_coords[j,i]-delta_lon;
            lon_vertices[j,i,1] = lon_coords[j,i];
            lon_vertices[j,i,2] = lon_coords[j,i]+delta_lon;
            lon_vertices[j,i,3] = lon_coords[j,i];
## !!$      /* vertices lat */
            lat_vertices[j,i,0] = lat_coords[j,i];
            lat_vertices[j,i,1] = lat_coords[j,i]-delta_lat;
            lat_vertices[j,i,2] = lat_coords[j,i];
            lat_vertices[j,i,3] = lat_coords[j,i]+delta_lat;
    print lat_vertices.min(),'---------------------'
    return x,y,lon_coords,lat_coords,lon_vertices,lat_vertices




myaxes=numpy.zeros(9,dtype='i')
myaxes2=numpy.zeros(9,dtype='i')
myvars=numpy.zeros(9,dtype='i')


cmor.setup(inpath="Tables",set_verbosity=cmor.CMOR_NORMAL, netcdf_file_action = cmor.CMOR_REPLACE, exit_control = cmor.CMOR_EXIT_ON_MAJOR);
cmor.dataset_json("Test/common_user_input.json")

tables=[]
a = cmor.load_table("CMIP6_grids.json")
tables.append(a)
tables.append(cmor.load_table("CMIP6_Omon.json"))
print 'Tables ids:',tables

cmor.set_table(tables[0])

x,y,lon_coords,lat_coords,lon_vertices,lat_vertices = gen_irreg_grid(lon,lat)
print lon_vertices.shape,lat_vertices.shape,x.shape,y.shape

myaxes[1] = cmor.axis(table_entry = 'y', 
                      units = 'm', 
                      coord_vals = y)
myaxes[0] = cmor.axis(table_entry = 'x', 
                      units = 'm', 
                      coord_vals = x)

print 'lons:',lon_vertices.shape,lon_coords.shape
grid_id = cmor.grid(axis_ids = myaxes[:2], 
                    latitude = lat_coords, 
                    longitude = lon_coords, 
                    latitude_vertices = lat_vertices, 
                    longitude_vertices = lon_vertices)
print 'got grid_id:',grid_id
myaxes[2] = grid_id

## mapnm = 'lambert_conformal_conic'
## params = [ "standard_parallel1",
##            "longitude_of_central_meridian","latitude_of_projection_origin",
##            "false_easting","false_northing","standard_parallel2" ]
## punits = ["","","","","","" ]
## pvalues = [-20.,175.,13.,8.,0.,20. ]
## cmor.set_grid_mapping(grid_id=myaxes[2],
##                       mapping_name = mapnm,
##                       parameter_names = params,
##                       parameter_values = pvalues,
##                       parameter_units = punits)

cmor.set_table(tables[1])
myaxes[3] = cmor.axis(table_entry = 'time',
                      units = 'months since 1980')
# Now sets up the ocn sigma stuff
levs=-numpy.arange(lev)/float(lev+1.)
blevs=-numpy.arange(lev+1)/float(lev+1.)
print 'Defining zlevs'
myaxes[4] = cmor.axis(table_entry='ocean_sigma',coord_vals=levs,cell_bounds=blevs,units='1')

print 'definnig zfactor depth',myaxes[2]
depth = numpy.random.random((lon,lat))*5000.
print 'Depth:',depth.shape,depth.dtype
idpth = cmor.zfactor(zaxis_id=myaxes[4],units='m',zfactor_name='depth',axis_ids=numpy.array([myaxes[2],]),zfactor_values=depth)

print 'defining zfactor eta'
ieta = cmor.zfactor(zaxis_id=myaxes[4],units='m',zfactor_name='eta',axis_ids=[myaxes[2],myaxes[3]])
print 'ieta:',ieta
pass_axes = [myaxes[4],myaxes[2],myaxes[3]]
print 'defining variable'
myvars[0] = cmor.variable( table_entry = 'thetao',
                           units = 'K',
                           axis_ids = pass_axes,
                           positive = 'down'
                           )
Time = numpy.zeros(ntimes,dtype='d')
bnds_time = numpy.zeros(ntimes*2,dtype='d')
Time[0],bnds_time[0:2] = read_time(0)
Time[1],bnds_time[2:4] = read_time(1)
for i in range(ntimes):
    data3d = numpy.random.random((lev,lon,lat,ntimes))*40.+273.15
    eta = numpy.random.random((lon,lat,ntimes))*10000.
    #print 'writing time: ',i,data3d.shape,data3d
    #print Time[i],bnds_time[2*i:2*i+2]
    print 'Writing time',i,'for var',data3d.shape
    cmor.write(myvars[0],data3d,1,time_vals=Time[i],time_bnds=bnds_time[2*i:2*i+2])
    print 'Writing time',i,'for eta'
    cmor.write(ieta,eta,1,time_vals=Time[i],time_bnds=bnds_time[2*i:2*i+2],store_with=myvars[0])
cmor.close()
