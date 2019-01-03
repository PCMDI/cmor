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
import os
import unittest
import base_test_cmor_python
import sys
import tempfile


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testCMIP6(self):
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
            cmor.setup(inpath=self.tabledir, netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "CMOR_input_example.json"))
            cmor.set_cur_dataset_attribute("experiment_id", "ssp434")
            cmor.set_cur_dataset_attribute(
                "parent_experiment_id", "historical")
            cmor.set_cur_dataset_attribute("parent_activity_id", "CMIP")
            cmor.set_cur_dataset_attribute("activity_id", "ScenarioMIP")
            cmor.set_cur_dataset_attribute("source_type", "AOGCM")
            cmor.set_cur_dataset_attribute("sub_experiment_id", "none")
            cmor.set_cur_dataset_attribute(
                "parent_variant_label", "r11i123p4556f333")
            cmor.set_cur_dataset_attribute("parent_source_id", "child")
            cmor.set_cur_dataset_attribute("parent_mip_era", "CMIP6")

            # ------------------------------------------
            # load Omon table and create masso variable
            # ------------------------------------------
            cmor.load_table("CMIP6_Omon.json")
            cmor.load_table("CMIP6_Amon.json")
            cmor.load_table("CMIP6_6hrPlev.json")
            cmor.load_table("CMIP6_6hrPlevPt.json")
            cmor.load_table("CMIP6_AERday.json")
            cmor.load_table("CMIP6_AERfx.json")
            cmor.load_table("CMIP6_AERhr.json")
            cmor.load_table("CMIP6_AERmon.json")
            cmor.load_table("CMIP6_AERmonZ.json")
            cmor.load_table("CMIP6_Amon.json")
            cmor.load_table("CMIP6_CFday.json")
            cmor.load_table("CMIP6_CFmon.json")
            cmor.load_table("CMIP6_CFsubhr.json")
            cmor.load_table("CMIP6_CFsubhrOff.json")
            cmor.load_table("CMIP6_day.json")
            cmor.load_table("CMIP6_E1hrClimMon.json")
            cmor.load_table("CMIP6_E1hr.json")
            cmor.load_table("CMIP6_E3hr.json")
            cmor.load_table("CMIP6_E6hrZ.json")
            cmor.load_table("CMIP6_EdayZ.json")
            cmor.load_table("CMIP6_Efx.json")
            cmor.load_table("CMIP6_EmonZ.json")
            cmor.load_table("CMIP6_Esubhr.json")
            cmor.load_table("CMIP6_Eyr.json")
            cmor.load_table("CMIP6_fx.json")
            cmor.load_table("CMIP6_grids.json")
            cmor.close()
            self.processLog()
        except BaseException:
            raise
            
        os.dup2(newstdout, 1)
        os.dup2(newstderr, 2)
        sys.stdout = os.fdopen(newstdout, 'w', 0)
        sys.stderr = os.fdopen(newstderr, 'w', 0)
        f = open(tmpfile[1], 'r')
        lines = f.readlines()
        for line in lines:
            if line.find('Error:') != -1:
                self.assertIn('30', line.strip())
                break
        f.close()

        os.unlink(tmpfile[1])


if __name__ == '__main__':
    unittest.main()
