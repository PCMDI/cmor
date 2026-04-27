import unittest
import os
import tempfile
import glob

debug = False


class BaseCVsTest(unittest.TestCase):

    def setUp(self, *args, **kwargs):
        # --------------
        # Create tmpfile
        # --------------
        self.tmpfile = tempfile.mkstemp()[1]
        if debug:
            print("TEMP:", self.tmpfile)
        self.delete_files = []

    def tearDown(self):
        if debug:
            print("would be unlinking:", self.tmpfile)
            return
        os.unlink(self.tmpfile)
        for filename in self.delete_files:
            os.remove(filename)
            self.delete_files.remove(filename)

    def assertCV(self, text_to_find, line_trigger='Error:', number_of_lines_to_scan=1):
        line_to_scan = ""
        if debug:
            print("LINE TRIGGER:", line_trigger)
        with open(self.tmpfile) as f:
            lines = f.readlines()
            for i, line in enumerate(lines):
                scan = "".join(lines[i:i+number_of_lines_to_scan]).strip()
                if debug: print("LINE:", i, __file__, scan)
                if line_trigger in line:
                    line_to_scan = scan
                    break
        if debug:
            print("SCANNED:", line_to_scan)
        self.assertIn(text_to_find, line_to_scan)