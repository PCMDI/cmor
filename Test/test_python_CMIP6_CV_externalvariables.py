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

import cmor
import numpy
import unittest
import os
import sys
import tempfile
import cdms2


# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


class TestCase(unittest.TestCase):
    def setUp(self, *args, **kwargs):
        # ------------------------------------------------------
        # Copy stdout and stderr file descriptor for cmor output
        # ------------------------------------------------------
        self.newstdout = os.dup(1)
        self.newstderr = os.dup(2)
        # --------------
        # Create tmpfile
        # --------------
        self.tmpfile = tempfile.mkstemp()
        os.dup2(self.tmpfile[0], 1)
        os.dup2(self.tmpfile[0], 2)
        os.close(self.tmpfile[0])

    def testCMIP6(self):

        nlat = 10
        dlat = 180. / nlat
        nlon = 20
        dlon = 360. / nlon
        nlev = 5
        ntimes = 5

        lats = numpy.arange(90 - dlat / 2., -90, -dlat)
        blats = numpy.arange(90, -90 - dlat, -dlat)
        lons = numpy.arange(0 + dlon / 2., 360., dlon)
        blons = numpy.arange(0, 360. + dlon, dlon)

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json("Test/common_user_input.json")

        # --------------------------------------------
        # load Omon table and create masscello variable
        # --------------------------------------------
        cmor.load_table("CMIP6_Omon.json")
        itime = cmor.axis(table_entry="time", units='months since 2010',
                          coord_vals=numpy.array([0, 1, 2, 3, 4.]),
                          cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
        ilat = cmor.axis(table_entry='latitude', coord_vals=lats, cell_bounds=blats, units='degrees_north')
        ilon = cmor.axis(table_entry='longitude', coord_vals=lons, cell_bounds=blons, units='degrees_east')
        ilev = cmor.axis(table_entry='depth_coord', length=5,
                         cell_bounds=numpy.arange(0, 12000, 2000), coord_vals=numpy.arange(0, 10000, 2000), units="m")

        ivar = cmor.variable(table_entry="masscello", axis_ids=[itime, ilev, ilat, ilon, ], units='kg/m2')

        data = numpy.random.random((ntimes, nlev, nlat, nlon)) * 100.

        cmor.write(ivar, data)
        cmor.close()
        os.dup2(self.newstdout, 1)
        os.dup2(self.newstderr, 2)
        sys.stdout = os.fdopen(self.newstdout, 'w', 0)
        sys.stderr = os.fdopen(self.newstderr, 'w', 0)

        f = cdms2.open(cmor.get_final_filename(), "r")
        a = f.getglobal("external_variables")
        self.assertEqual("areacello volcello", a)

    def tearDown(self):                                                                                                                        
        import shutil                                                                                                                          
        shutil.rmtree("./CMIP6")                                                                                                               


if __name__ == '__main__':
    run()
