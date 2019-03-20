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
from __future__ import print_function
import cmor
import numpy
import unittest
import os
import sys
import tempfile
import cdms2
import base_CMIP6_CV

# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


class TestCase(base_CMIP6_CV.BaseCVsTest):
    def tstCMIP6(self):

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
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")

        # --------------------------------------------
        # load Omon table and create masscello variable
        # --------------------------------------------
        cmor.load_table("CMIP6_Omon.json")
        itime = cmor.axis(table_entry="time", units='months since 2010',
                          coord_vals=numpy.array([0, 1, 2, 3, 4.]),
                          cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
        ilat = cmor.axis(
            table_entry='latitude',
            coord_vals=lats,
            cell_bounds=blats,
            units='degrees_north')
        ilon = cmor.axis(
            table_entry='longitude',
            coord_vals=lons,
            cell_bounds=blons,
            units='degrees_east')
        ilev = cmor.axis(table_entry='depth_coord', length=5,
                         cell_bounds=numpy.arange(0, 12000, 2000), coord_vals=numpy.arange(0, 10000, 2000), units="m")

        ivar = cmor.variable(
            table_entry="masscello", axis_ids=[
                itime, ilev, ilat, ilon, ], units='kg/m2')

        data = numpy.random.random((ntimes, nlev, nlat, nlon)) * 100.

        cmor.write(ivar, data)
        self.delete_files += [cmor.close(ivar, True)]
        cmor.close()

        f = cdms2.open(cmor.get_final_filename(), "r")
        a = f.getglobal("external_variables")
        self.assertEqual("areacello volcello", a)

    def testCMIP6_ExternaVariablesError(self):

        print("CMIP6 External Error")
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
        error_flag = cmor.dataset_json('Test/CMOR_input_example.json')
        table_id = cmor.load_table('CMIP6_6hrLev.json')
        time = cmor.axis(table_entry='time1', units='days since 2000-01-01',
                        coord_vals=numpy.array(range(1)),
                        cell_bounds=numpy.array(range(2)))
        latitude = cmor.axis(table_entry='latitude', units='degrees_north',
                            coord_vals=numpy.array(range(5)),
                            cell_bounds=numpy.array(range(6)))
        longitude = cmor.axis(table_entry='longitude', units='degrees_east',
                            coord_vals=numpy.array(range(5)),
                            cell_bounds=numpy.array(range(6)))
        plev3 = cmor.axis(table_entry='plev3', units='Pa',
                        coord_vals=numpy.array([85000., 50000., 25000.]))
        axis_ids = [longitude, latitude, plev3, time]
        ua_var_id = cmor.variable(table_entry='ua', axis_ids=axis_ids,
                                units='m s-1')
        ta_var_id = cmor.variable(table_entry='ta', axis_ids=axis_ids,
                                units='K')
        data = numpy.random.random(75)
        reshaped_data = data.reshape((5, 5, 3, 1))

        # This doesn't:
        cmor.write(ta_var_id, reshaped_data)
        cmor.write(ua_var_id, reshaped_data)
        fname_ta = cmor.close(ta_var_id, file_name=True)
        fname_ua = cmor.close(ua_var_id, file_name=True)
        self.delete_files += [cmor.close(ta_var_id, True)]
        self.delete_files += [cmor.close(ua_var_id, True)]
        cmor.close()

        f = cdms2.open(fname_ta, "r")
        a = f.getglobal("external_variables")
        self.assertEqual("areacella", a)
        f.close()
        f = cdms2.open(fname_ua, "r")
        a = f.getglobal("external_variables")
        self.assertEqual(None, a)
        f.close()


if __name__ == '__main__':
    run()
