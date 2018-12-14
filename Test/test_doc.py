import cmor

cmor.setup(
	inpath='./cmip6-cmor-tables/Tables',  # this has to point to the CMIP6 CMOR tables path
	netcdf_file_action=cmor.CMOR_REPLACE_4
)

cmor.dataset_json("common_user_input.json")    
  
# Loading this test table overwrites the normal CF checks on valid variable values.
# This is perfect for testing but shouldn't be done when writing real data.
table='CMIP6_Amon.json'
cmor.load_table(table)


# here is where you add your axes
itime = cmor.axis(table_entry= 'time',
                  units= 'days since 2000-01-01 00:00:00',
                  coord_vals= [15,],
                  cell_bounds= [0, 30])
ilat = cmor.axis(table_entry= 'latitude',
                 units= 'degrees_north',
                 coord_vals= [0],
                 cell_bounds= [-1, 1])
ilon = cmor.axis(table_entry= 'longitude',
                 units= 'degrees_east',
                 coord_vals= [90],
                 cell_bounds= [89, 91])

axis_ids = [itime,ilat,ilon]
              
# here we create a variable with appropriate name, units and axes
varid = cmor.variable('ts', 'K', axis_ids)

# then we can write the variable along with the data
cmor.write(varid, [273])

# finally we close the file and print where it was saved
outfile = cmor.close(varid, file_name=True)
print("File written to: {}".format(outfile))
cmor.close()
