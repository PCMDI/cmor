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
import json
import signal
import unittest

def run():
    unittest.main()

class TestdirectoryMethods(base_CMIP6_CV.BaseCVsTest):

    def bad_outpath(self):
        override = os.environ.get("CMOR_BAD_DIRECTORY_OUTPATH")
        if override:
            return override

        # cibuildwheel Linux tests run as root inside the manylinux container,
        # so the historical "/CMIP6" path is writable there.
        if hasattr(os, "geteuid") and os.geteuid() == 0 and sys.platform.startswith("linux"):
            return "/proc/CMIP6"

        return "/CMIP6"

    def test_Directory(self):

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
        with open("Test/baddirectory.json") as source:
            dataset = json.load(source)
        dataset["outpath"] = self.bad_outpath()
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as tmp_json:
            json.dump(dataset, tmp_json)
            tmp_json_path = tmp_json.name
        self.addCleanup(lambda: os.path.exists(tmp_json_path) and os.unlink(tmp_json_path))
        try:
            cmor.dataset_json(tmp_json_path)
        except BaseException:
            pass
        self.assertCV("unable to create this directory")

if __name__ == '__main__':
    run()
