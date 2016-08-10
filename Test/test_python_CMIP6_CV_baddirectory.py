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

from threading import Thread
import time
import cmor
import numpy
import unittest
import signal
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
os.close(tmpfile[0])

global testOK
testOK = []

# --------------
# Create thread
# --------------


def sig_handler(signum, frame):

    global testOK

    f = open(tmpfile[1], 'r')
    lines = f.readlines()
    for line in lines:
        if line.find('Error:') != -1:
            testOK = line.strip()
            break
    f.close()
#    os.unlink(tmpfile[1])


def run():
    unittest.main()

signal.signal(signal.SIGINT, sig_handler)
signal.signal(signal.SIGINT, sig_handler)


class TestdirectoryMethods(unittest.TestCase):

    def test_Directory(self):

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        global testOK
        error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
        error_flag = cmor.dataset_json("Test/test_python_CMIP6_CV_baddirectory.json")

        os.dup2(newstdout, 1)
        os.dup2(newstderr, 2)
        sys.stdout = os.fdopen(newstdout, 'w', 0)
        time.sleep(1)
        self.assertIn("unable to create this directory", testOK)


if __name__ == '__main__':
    t = Thread(target=run)
    t.start()
    while t.is_alive():
        t.join(1)
