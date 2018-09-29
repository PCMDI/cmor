import unittest
import os
import re
import shutil
import tempfile


class BaseCmorTest(unittest.TestCase):

    def setUp(self, *args, **kwargs):
        # get directories for test files and tables
        self.curdir = os.getcwd()
        self.testdir = os.path.join(self.curdir, 'Test')
        self.assertEqual(os.path.isdir(self.testdir), True, '%s is not a directory.'%(self.testdir))
        self.tabledir = os.path.join(self.curdir, 'Tables')
        self.assertEqual(os.path.isdir(self.tabledir), True, '%s is not a directory.'%(self.tabledir))

        # switch to temporary directory
        test_id = self.id().split('.')
        test_file = test_id[1]
        test_case = test_id[3]
        self.tempdir = tempfile.mkdtemp(prefix=test_file)
        os.chdir(self.tempdir)
        self.logfile = os.path.join(self.tempdir, '.'.join([test_case, 'log']))

    def tearDown(self):
        # get out of temporary directory
        os.chdir(self.curdir)

        # delete temporary directory
        shutil.rmtree(self.tempdir)

    def processLog(self):
        # find errors in CMOR log
        remove_ansi = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
        msg_pattern = 'C Traceback:[\\s\\S]*?!!!!!!!!!!!!!!!!!!!!!!!!!\n\n'
        with open(self.logfile) as f:
            lines = f.readlines()
            log_text = remove_ansi.sub('', ''.join(lines))
            messages = re.findall(msg_pattern, log_text)
            for msg in messages:
                self.assertNotIn('! Error: ', msg, 'CMOR ERROR:\n %s'%(msg))