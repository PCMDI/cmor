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
import base_CMIP6_CV
import sys
import os
import tempfile
import signal
import unittest

def run():
    unittest.main()

class TestdirectoryMethods(base_CMIP6_CV.BaseCVsTest):

    def test_Directory(self):

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile[1])
        try:
            cmor.dataset_json("Test/baddirectory.json")
        except (KeyboardInterrupt, BaseException):
            pass
        self.assertCV("unable to create this directory")

if __name__ == '__main__':
    run()
