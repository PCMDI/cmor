# If this example is not executed from the directory containing the
# CMOR code, please first complete the following steps:
#
#   1. In any directory, create 'Tables/', 'Test/' and 'CMIP6/' directories.
#
#   2. Download
#      https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_Omon.json
#      and https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_CV.json
#      to the 'Tables/' directory.
#
#   3. Download
#      https://github.com/PCMDI/cmor/blob/master/Test/<filename>.json
#      to the 'Test/' directory.


import cmor,numpy

error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
  
error_flag = cmor.dataset_json("Test/test_python_CMIP6_wrong_activity.json")
  
cmor.load_table("CMIP6_Omon.json")
itime = cmor.axis(table_entry="time",units='months since 2010',coord_vals=numpy.array([0,1,2,3,4.]),cell_bounds=numpy.array([0,1,2,3,4,5.]))
ivar = cmor.variable(table_entry="masso",axis_ids=[itime],units='kg')
data=numpy.random.random(5)
for i in range(0,5):
    cmor.write(ivar,data[i:i])#,time_vals=numpy.array([i,]),time_bnds=numpy.array([i,i+1]))
error_flag = cmor.close()

