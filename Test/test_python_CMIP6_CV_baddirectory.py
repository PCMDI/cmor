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
import unittest
import sys
import os
import tempfile


def run():
    unittest.main()


class TestdirectoryMethods(unittest.TestCase):

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

    def getAssertTest(self):
        f = open(self.tmpfile[1], 'r')
        lines = f.readlines()
        for line in lines:
            if line.find('Error:') != -1:
                testOK = line.strip()
                break
        f.close()
        os.unlink(self.tmpfile[1])
        return testOK

    def test_Directory(self):

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
        try:
            cmor.dataset_json("Test/common_user_input.json")
        except:
            testOK = self.getAssertTest()
            os.dup2(self.newstdout, 1)
            os.dup2(self.newstderr, 2)
            sys.stdout = os.fdopen(self.newstdout, 'w', 0)
            sys.stderr = os.fdopen(self.newstderr, 'w', 0)
            self.assertIn("unable to create this directory", testOK)


if __name__ == '__main__':
    run()
