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
import base_CMIP6_CV


# ==============================
#  main thread
# ==============================


def run():
    unittest.main()


class TestCase(base_CMIP6_CV.BaseCVsTest):

    def testCMIP6(self):

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        nlat = 10
        dlat = 180. / nlat
        nlon = 20
        dlon = 360. / nlon

        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_fx.json")

        lats = numpy.arange(90 - dlat / 2., -90, -dlat)
        blats = numpy.arange(90, -90 - dlat, -dlat)
        lons = numpy.arange(0 + dlon / 2., 360., dlon)
        blons = numpy.arange(0, 360. + dlon, dlon)

        data = lats[:, numpy.newaxis] * lons[numpy.newaxis, :]

        data = (data + 1e10) / 750. + 233.2

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

        # ------------------------------------------
        # load Omon table and create masso variable
        # ------------------------------------------
        ivar = cmor.variable(
            table_entry="areacella", axis_ids=[
                ilat, ilon], units='m2')

        cmor.write(ivar, data)
        self.delete_files += [cmor.close(ivar, True)]
        cmor.close()


if __name__ == '__main__':
    run()
