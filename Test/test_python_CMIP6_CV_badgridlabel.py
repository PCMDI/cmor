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


# ------------------------------------------------------
# Copy stdout and stderr file descriptor for cmor output
# ------------------------------------------------------
newstdout = os.dup(1)
newstderr = os.dup(2)
# --------------
# Create tmpfile
# --------------
tmpfile = tempfile.mkstemp()
os.dup2(tmpfile[0], 1)
os.dup2(tmpfile[0], 2)


# ==============================
#  getAssertTest()
# ==============================
def getAssertTest():

    f = open(tmpfile[1], 'r')
    lines = f.readlines()
    for line in lines:
        if line.find('Error:') != -1:
            testOK = line.strip()
            break
    f.close()
    os.unlink(tmpfile[1])
    return testOK


# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


# ---------------------
# Hook up SIGTEM signal
# ---------------------


class TestInstitutionMethods(unittest.TestCase):

    def testCMIP6(self):
        try:
            # -------------------------------------------
            # Try to call cmor with a bad institution_ID
            # -------------------------------------------
            cmor.setup(
                inpath='Tables',
                netcdf_file_action=cmor.CMOR_REPLACE)

            cmor.dataset_json("Test/test_python_CMIP6_CV_badgridlabel.json")

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
        except:
            os.dup2(newstdout, 1)
            os.dup2(newstderr, 2)
            testOK = getAssertTest()

            sys.stdout = os.fdopen(newstdout, 'w', 0)
            sys.stderr = os.fdopen(newstderr, 'w', 0)
            # ------------------------------------------
            # Check error after signal handler is back
            # ------------------------------------------
            self.assertIn("\"gs1n\"", testOK)


if __name__ == '__main__':
    run()
