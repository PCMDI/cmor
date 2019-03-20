from __future__ import print_function
from common import *
import cmor


cmor.setup(inpath="Test",
           set_verbosity=cmor.CMOR_NORMAL,
           netcdf_file_action=cmor.CMOR_REPLACE)

cmor.dataset_json("Test/metadata-template.json")
cmor.set_cur_dataset_attribute("calendar", "gregorian")
cmor.load_table("Tables/CMIP6_Amon.json")
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

itim = cmor.axis(
    table_entry="time",
    units="days since 1850",
    length=ntimes)

var2d_ids = []
for m in ["TSURF", ]:
    var2d_ids.append(
        cmor.variable(table_entry=specs[m]["entry"],
                      units=specs[m]["units"],
                      axis_ids=[itim, ilat, ilon],
                      missing_value=1.e28,
                      positive=specs[m]["positive"],
                      original_name=m)
    )

for index in range(ntimes):
    tim_array, bnds_tim = read_time(index)
    for i, varname in enumerate(["TSURF", ]):
        data = read_2d_input_files(index, varname, (lat, lon))
        print(data.shape, data)
        print(tim_array, bnds_tim)
        cmor.write(var_id=var2d_ids[i], data=data, ntimes_passed=1,
                   time_vals=tim_array, time_bnds=bnds_tim)
        print("Passed write")

cmor.close()
