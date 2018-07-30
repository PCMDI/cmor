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
import base_CMIP6_CV


# ==============================
#  main thread
# ==============================


def run():
    unittest.main()


class TestCase(base_CMIP6_CV.BaseCVsTest):

    def testCMIP6(self):
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
            cmor.dataset_json("Test/common_user_input_hier.json")

            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time", units='months since 2010', coord_vals=numpy.array(
                [0, 1, 2, 3, 4.]), cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
            ivar = cmor.variable(
                table_entry="masso",
                axis_ids=[itime],
                units='kg')

            data = numpy.random.random(5)
            for i in range(0, 5):
                # ,time_vals=numpy.array([i,]),time_bnds=numpy.array([i,i+1]))
                cmor.write(ivar, data[i:i])
            self.delete_files += [cmor.close(ivar, True)]
            cmor.close()

            f = cdms2.open(cmor.get_final_filename() , 'r')
            self.assertEqual(f.coder, "Denis Nadeau")
            self.assertEqual(f.hierarchical_attr_setting, "information")
            self.assertEqual(f.creator, "PCMDI")
            self.assertEqual(f.model, "Ocean Model")
            self.assertEqual(f.country, "USA")
            f.close()
        except KeyboardInterrupt:
            raise RuntimeError("Unexpected Error")

if __name__ == '__main__':
    run()
