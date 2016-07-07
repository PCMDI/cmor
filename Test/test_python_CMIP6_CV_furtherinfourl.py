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
import cmor,numpy
import contextlib
import unittest
import signal
import sys,os
import tempfile
import atexit
import cdms2
import pdb


# ------------------------------------------------------
# Copy stdout and stderr file descriptor for cmor output
# ------------------------------------------------------
newstdout = os.dup(1)
newstderr = os.dup(2)
# --------------
# Create tmpfile
# --------------
tmpfile = tempfile.mkstemp() #tempfile[0] = File number, tempfile[1] = File name.
os.dup2(tmpfile[0], 1)
os.dup2(tmpfile[0], 2)
os.close(tmpfile[0])

global testOK 
testOK = []

# ==============================
# Handle SIGINT receive by CMOR
# ==============================
def sig_handler(signum, frame):
    global testOK

    f=open(tmpfile[1],'r')
    lines=f.readlines()
    for line in lines:
        if line.find('Error:') != -1:
            testOK = line.strip()
            break
    f.close()
    os.unlink(tmpfile[1])


# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


# ---------------------
# Hook up SIGTEM signal 
# ---------------------
signal.signal(signal.SIGINT, sig_handler)


class TestInstitutionMethods(unittest.TestCase):

    def testCMIP6(self):
        ''' This test will not fail we veirfy the attribute further_info_url'''

        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        global testOK
        error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
        error_flag = cmor.dataset_json("Test/test_python_CMIP6_CV_furtherinfourl.json")
  
        # ------------------------------------------
        # load Omon table and create masso variable
        # ------------------------------------------
        cmor.load_table("CMIP6_Omon.json")
        itime = cmor.axis(table_entry="time",units='months since 2010',
                          coord_vals=numpy.array([0,1,2,3,4.]),
                          cell_bounds=numpy.array([0,1,2,3,4,5.]))
        ivar = cmor.variable(table_entry="masso",axis_ids=[itime],units='kg')

        data=numpy.random.random(5)
        for i in range(0,5):
            a = cmor.write(ivar,data[i:i])
        file = cmor.close()
        print file
        os.dup2(newstdout,1)
        os.dup2(newstderr,2)
        sys.stdout = os.fdopen(newstdout, 'w', 0)
        sys.stderr = os.fdopen(newstderr, 'w', 0)
        f=cdms2.open(cmor.get_final_filename(),"r")
        a=f.getglobal("further_info_url")
        self.assertEqual("http://furtherinfo.es-doc.org/CMIP6/NCC.MIROC-ESM.piControl-withism.s1968.r1i1p1f1", a)



if __name__ == '__main__':
    t = Thread(target=run)
    t.start()
    while t.is_alive():
        t.join(1)


