import cdms2
import cmor
import numpy
import os
import common

varin3d = ["MC", ]

n3d = len(varin3d)

pth = os.getcwd()
common.init_cmor(pth, "CMOR_input_example.json")
ierr = cmor.load_table(os.path.join(pth, 'Tables', 'CMIP6_Amon.json'))
itim, ilat, ilon = common.read_cmor_time_lat_lon()

ilev_half = cmor.axis(table_entry='standard_hybrid_sigma_half',
                      units='1',
                      coord_vals=common.zlevs)
print("ILEVL half:", ilev_half)

cmor.zfactor(zaxis_id=ilev_half, zfactor_name='p0',
             units='hPa', zfactor_values=common.p0)
print "p0 1/2"
cmor.zfactor(zaxis_id=ilev_half, zfactor_name='b_half', axis_ids=[ilev_half, ],
             zfactor_values=common.b_coeff)
print "b 1/2"
cmor.zfactor(zaxis_id=ilev_half, zfactor_name='a_half', axis_ids=[ilev_half, ],
             zfactor_values=common.a_coeff)
print "a 1/2"
ps_var = cmor.zfactor(zaxis_id=ilev_half, zfactor_name='ps',
                      axis_ids=[ilon, ilat, itim], units='hPa')
print "ps 1/2"

var3d_ids = []
for m in varin3d:
    print "3d VAR:", m
    var3d_ids.append(
        cmor.variable(table_entry=common.specs[m]["entry"],
                      units=common.specs[m]["units"],
                      axis_ids=[itim, ilev_half, ilat, ilon],
                      missing_value=1.e28,
                      positive=common.specs[m]["positive"],
                      original_name=m)
    )
print "Ok now writing", var3d_ids, common.ntimes
for index in range(common.ntimes):
    tim_array, bnds_tim = common.read_time(index)
    for i, varname in enumerate(varin3d):
        data = common.read_3d_input_files(
            index, varname, (common.lev, common.lat, common.lon))
        print data.shape, data
        print tim_array, bnds_tim
        cmor.write(var_id=var3d_ids[i], data=data, ntimes_passed=1,
                   time_vals=tim_array, time_bnds=bnds_tim)
        print("Passed write")
        # PSURF
        data = common.read_2d_input_files(
            index, "PSURF", (common.lat, common.lon))
        cmor.write(var_id=ps_var, data=data, ntimes_passed=1,
                   time_vals=tim_array, time_bnds=bnds_tim, store_with=var3d_ids[i])
cmor.close()
