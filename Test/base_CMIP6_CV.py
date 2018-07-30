from __future__ import print_function
import cmor
import unittest
import sys
import os
import tempfile
import signal
import glob

debug = False

class BaseCVsTest(unittest.TestCase):


    def remove_file_and_directories(self, filename):
        os.remove(filename)
        filename = os.path.dirname(filename)
        while glob.glob(os.path.join(filename, "*")) == []:
            os.rmdir(filename)
            filename = os.path.dirname(filename)

    def signal_handler(self, sig, frame):
        if debug: print("Code received SIGINT")
        return

    def setUp(self, *args, **kwargs):
        
        # --------------
        # Create sigint handler
        # --------------
        signal.signal(signal.SIGINT, self.signal_handler)
        # --------------
        # Create tmpfile
        # --------------
        self.tmpfile = tempfile.mkstemp()[1]
        if debug: print("TEMP:",self.tmpfile)
        self.delete_files = []

    def tearDown(self):
        if debug: print("unlinking:",self.tmpfile)
        os.unlink(self.tmpfile)
        for filename in self.delete_files:
            self.remove_file_and_directories(filename)
            self.delete_files.remove(filename)

    def assertCV(self, text_to_find, line_trigger='Error:', number_of_lines_to_scan=1):
        line_to_scan = ""
        if debug: print("LINE TRIGGER:", line_trigger)
        with open(self.tmpfile) as f:
            lines = f.readlines()
            for i, line in enumerate(lines):
                scan = "".join(lines[i:i+number_of_lines_to_scan]).strip()
                if debug: print("LINE:",scan)
                if line_trigger in line:
                    line_to_scan = scan
                    break
        if debug: print("SCANNED:",line_to_scan)
        self.assertIn(text_to_find, line_to_scan)