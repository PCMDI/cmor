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

from threading import Thread
import time
import cmor,numpy
import contextlib
import unittest
import signal
import sys,os
import tempfile
import cdms2



# ==============================
#  main thread
# ==============================
def run():
    unittest.main()



class TestInstitutionMethods(unittest.TestCase):

    def testCMIP6(self):

        nlat = 10
        dlat = 180./nlat
        nlon = 20
        dlon = 360./nlon
        nlev =5 
        ntimes = 5

        lats = numpy.arange(90-dlat/2.,-90,-dlat)
        blats = numpy.arange(90,-90-dlat,-dlat)
        lons = numpy.arange(0+dlon/2.,360.,dlon)
        blons = numpy.arange(0,360.+dlon,dlon)

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        global testOK
        error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
        error_flag = cmor.dataset_json("Test/test_python_CMIP6_CV_externalvariables.json")
  
        # --------------------------------------------
        # load Omon table and create masscello variable
        # --------------------------------------------
        cmor.load_table("CMIP6_Omon.json")
        itime = cmor.axis(table_entry="time",units='months since 2010',
                          coord_vals=numpy.array([0,1,2,3,4.]),
                          cell_bounds=numpy.array([0,1,2,3,4,5.]))
        ilat = cmor.axis(table_entry='latitude', coord_vals=lats, cell_bounds=blats,units='degrees_north')
        ilon = cmor.axis(table_entry='longitude', coord_vals=lons, cell_bounds=blons,units='degrees_east')
        ilev = cmor.axis(table_entry='depth_coord', length=5,
                         cell_bounds=numpy.arange(0,12000,2000), coord_vals=numpy.arange(0,10000,2000), units="m")

        ivar = cmor.variable(table_entry="masscello",axis_ids=[itime, ilev, ilat,ilon,],units='kg/m2')

        data = numpy.random.random((ntimes,nlev,nlat,nlon))*100.

        a = cmor.write(ivar,data)
        cmor.close()
        f=cdms2.open(cmor.get_final_filename(),"r")
        a=f.getglobal("external_variables")
        self.assertEqual("areacello volcello", a)


if __name__ == '__main__':
    t = Thread(target=run)
    t.start()
    while t.is_alive():
        t.join(1)


