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
import signal

# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


class TestCase(unittest.TestCase):

    def testTerminateSignal(self):
        self.assertEqual(cmor.get_terminate_signal(), -999)
        cmor.setup()
        self.assertEqual(cmor.get_terminate_signal(), signal.SIGTERM)
        cmor.set_terminate_signal(signal.SIGINT)
        self.assertEqual(cmor.get_terminate_signal(), signal.SIGINT)

if __name__ == '__main__':
    run()