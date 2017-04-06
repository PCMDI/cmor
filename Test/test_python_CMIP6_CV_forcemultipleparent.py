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



class TestCase(unittest.TestCase):

    def testParentActivityID(self):

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
        os.close(tmpfile[0])
        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
            cmor.dataset_json("Test/common_user_input.json")
            cmor.set_cur_dataset_attribute("parent_experiment_id","dcppA-assim")
            cmor.set_cur_dataset_attribute("parent_activity_id","CMIP")
            cmor.set_cur_dataset_attribute("experiment_id","dcppC-forecast-addAgung")
            cmor.set_cur_dataset_attribute("activity_id","DCPP")
            cmor.set_cur_dataset_attribute("source_type","AOGCM AER")
            cmor.set_cur_dataset_attribute("sub_experiment_id","s2014")


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
            pass
        os.dup2(newstdout, 1)
        os.dup2(newstderr, 2)
        sys.stdout = os.fdopen(newstdout, 'w', 0)
        sys.stderr = os.fdopen(newstderr, 'w', 0)
        f = open(tmpfile[1], 'r')
        lines = f.readlines()
        for line in lines:
            if line.find('Error:') != -1:
                self.assertIn('parent_activity_id', line.strip())
                break
        f.close()
        os.unlink(tmpfile[1])

    def testParentExperimentID(self):

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
        os.close(tmpfile[0])
        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
            cmor.dataset_json("Test/common_user_input.json")
            cmor.set_cur_dataset_attribute("parent_experiment_id","historical")
            cmor.set_cur_dataset_attribute("parent_activity_id","DCPP")
            cmor.set_cur_dataset_attribute("experiment_id","dcppC-forecast-addAgung")
            cmor.set_cur_dataset_attribute("activity_id","DCPP")
            cmor.set_cur_dataset_attribute("source_type","AOGCM AER")
            cmor.set_cur_dataset_attribute("sub_experiment_id","s2014")


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
            pass
        os.dup2(newstdout, 1)
        os.dup2(newstderr, 2)
        sys.stdout = os.fdopen(newstdout, 'w', 0)
        sys.stderr = os.fdopen(newstderr, 'w', 0)
        f = open(tmpfile[1], 'r')
        lines = f.readlines()
        for line in lines:
            if line.find('Error:') != -1:
                self.assertIn('parent_experiment_id', line.strip())
                break
        f.close()
        os.unlink(tmpfile[1])

    def tearDown(self):
        import shutil
        shutil.rmtree("./CMIP6")


if __name__ == '__main__':
    unittest.main()
