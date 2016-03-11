
import cmor,numpy



vars ={'hfls' : ['W.m-2',25.,40.],'tas':['K',25,268.15],'clt':['%',100.,0.],'ta':['K',25,273.15]}


nlat = 90
dlat = 180/nlat
nlon = 180
dlon = 360./nlon
nlev = 17
ntimes = 12

lats = numpy.arange(-90+dlat/2.,90,dlat)
blats = numpy.arange(-90,90+dlat,dlat)
lons = numpy.arange(0+dlon/2.,360.,dlon)
blons = numpy.arange(0,360.+dlon,dlon)


tvars= ['hfls','tas','clt','ta']

cmor.setup(inpath='Tables',netcdf_file_action=cmor.CMOR_REPLACE)
cmor.dataset_json("Test/test_python_open_close_cmor_multiple.json")
table='CMIP6_Amon.json'
cmor.load_table(table)

for var in tvars:
    ilat = cmor.axis(table_entry='latitude',coord_vals=lats,cell_bounds=blats,units='degrees_north')
    ilon = cmor.axis(table_entry='longitude',coord_vals=lons,cell_bounds=blons,units='degrees_east')
    itim = cmor.axis(table_entry='time',coord_vals=numpy.arange(ntimes,dtype=numpy.float),cell_bounds=numpy.arange(ntimes+1,dtype=float),units='months since 2000')
    ilev = cmor.axis(table_entry='plev17',coord_vals=numpy.array([1000.,925,850,700,600,500,400,300,250,200,150,100,70,50,30,20,10]),units='hPa')
    
    if var!='ta':
        axes = [itim,ilat,ilon]
        data = numpy.random.random((ntimes,nlat,nlon))*vars[var][1]+vars[var][2]
    else:
        axes=[itim,ilev,ilat,ilon]
        data = numpy.random.random((ntimes,nlev,nlat,nlon))*vars[var][1]+vars[var][2]

    kw={}
    if var in ['hfss','hfls']:
        kw['positive']='up'
    var = cmor.variable(table_entry=var,units=vars[var][0],axis_ids=axes,**kw)

    cmor.write(var,data)
    path=cmor.close(var, file_name=True)
    print 'Saved in:',path

cmor.close()



print 'hello'
