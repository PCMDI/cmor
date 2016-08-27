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
import cdms2


# ------------------------------------------------------
# Copy stdout and stderr file descriptor for cmor output
# ------------------------------------------------------
newstdout = os.dup(1)
newstderr = os.dup(2)


# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


class TestInstitutionMethods(unittest.TestCase):

    def testCMIP6(self):
        ''' This test will not fail we veirfy the attribute further_info_url'''
        try:
            # -------------------------------------------
            # Try to call cmor with a bad institution_ID
            # -------------------------------------------
            global testOK
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
            cmor.dataset_json("Test/test_python_CMIP6_CV_furtherinfourl.json")

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
        except:
            raise

        f = cdms2.open(cmor.get_final_filename(), "r")
        a = f.getglobal("further_info_url")
        self.assertEqual("http://furtherinfo.es-doc.org/CMIP6.NCC.MIROC-ESM.piControl-withism.none.r1i1p1f1", a)


if __name__ == '__main__':
    run()
