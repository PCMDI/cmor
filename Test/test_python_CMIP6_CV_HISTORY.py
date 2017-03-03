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
import cdms2
import os
import sys
import tempfile


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
        ''' '''

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json("Test/common_user_input.json")
        cmor.load_table("CMIP6_Omon.json")

        cmor.set_cur_dataset_attribute("history", "set for CMIP6 unittest")

        # ------------------------------------------
        # load Omon table and create masso variable
        # ------------------------------------------
        cmor.load_table("CMIP6_Omon.json")
        itime = cmor.axis(table_entry="time", units='months since 2010',
                          coord_vals=numpy.array([0, 1, 2, 3, 4.]),
                          cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
        ivar = cmor.variable(table_entry="masso", axis_ids=[itime], units='kg')

        data = numpy.random.random(5)
        for i in range(0, 5):
            cmor.write(ivar, data[i:i])
        cmor.close()
        f = cdms2.open(cmor.get_final_filename(), "r")
        a = f.getglobal("history")
        self.assertIn("set for CMIP6 unittest", a)
        os.dup2(self.newstdout, 1)
        os.dup2(self.newstderr, 2)
        sys.stdout = os.fdopen(self.newstdout, 'w', 0)
        sys.stderr = os.fdopen(self.newstderr, 'w', 0)

    def tearDown(self):
        import shutil
        shutil.rmtree("./CMIP6")


if __name__ == '__main__':
    run()
