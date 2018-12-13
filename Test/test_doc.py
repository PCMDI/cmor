import cmor

cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE_4)

cmor.dataset_json("Test/CMOR_input_example.json")

table = 'CMIP6_Amon.json'
cmor.load_table(table)

itime = cmor.axis(table_entry='time',
                  units='days since 2000-01-01 00:00:00',
                  coord_vals=[15, ],
                  cell_bounds=[0, 30])
ilat = cmor.axis(table_entry='latitude',
                 units='degrees_north',
                 coord_vals=[0],
                 cell_bounds=[-1, 1])
ilon = cmor.axis(table_entry='longitude',
                 units='degrees_east',
                 coord_vals=[90],
                 cell_bounds=[89, 91])

axis_ids = [itime, ilat, ilon]

varid = cmor.variable('ts', 'K', axis_ids)
cmor.write(varid, [273])
outfile = cmor.close(varid, file_name=True)
print "File written: ", outfile
cmor.close()
