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


class TestCase(base_CMIP6_CV.BaseCVsTest):

    def testCMIP6(self):

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE,logfile=self.tmpfile)
            cmor.dataset_json("Test/CMOR_input_example.json")
            cmor.set_cur_dataset_attribute("experiment_id", "dcppA-hindcast")
            cmor.set_cur_dataset_attribute(
                "parent_experiment_id", "dcppA-assim")
            cmor.set_cur_dataset_attribute("parent_activity_id", "DCPP")
            cmor.set_cur_dataset_attribute("activity_id", "DCPP")
            cmor.set_cur_dataset_attribute("source_type", "AOGCM AER")
            cmor.set_cur_dataset_attribute("sub_experiment_id", "s3000")
            cmor.set_cur_dataset_attribute(
                "parent_variant_label", "r11i123p4556f333")
            cmor.set_cur_dataset_attribute(
                "parent_source_id", "PCMDI-test-1-0")
            cmor.set_cur_dataset_attribute("parent_mip_era", "CMIP6")

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
            for i in range(0, 5):
                cmor.write(ivar, data[i:i])
            self.delete_files += [cmor.close(ivar, True)]
            cmor.close()
        except BaseException:
            pass
        self.assertCV('sub_experiment_id')

if __name__ == '__main__':
    unittest.main()
