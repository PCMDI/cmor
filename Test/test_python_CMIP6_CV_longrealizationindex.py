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
import sys
import os
import tempfile
import base_CMIP6_CV

# ==============================
#  main thread
# ==============================
def run():
    return(unittest.main())


class TestCase(base_CMIP6_CV.BaseCVsTest):

    def testCMIP6(self):
        try:
            # -------------------------------------------
            # Try to call cmor with a bad institution_ID
            # -------------------------------------------
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
            cmor.dataset_json("Test/common_user_input.json")
            cmor.set_cur_dataset_attribute("initialization_index",
                                           "1209374928349823498274987234987")

            # ------------------------------------------
            # load Omon table and create masso variable
            # ------------------------------------------
            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time", units='months since 2010',
                              coord_vals=numpy.array([0, 1, 2, 3, 4.]),
                              cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
            ivar = cmor.variable(
                table_entry="masso",
                axis_ids=[itime],
                units='kg')

            data = numpy.random.random(5)
            for i in range(0, 1):
                cmor.write(ivar, data[i:i])
            cmor.close()

        except KeyboardInterrupt:
            raise RuntimeError("Unexpected Error")
        except (KeyboardInterrupt, BaseException):
            pass
        # ------------------------------------------
        # Check error after signal handler is back
        # ------------------------------------------
        self.assertCV("1209374928349823498274987234987")

if __name__ == '__main__':
    run()
