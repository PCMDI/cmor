import os
import sys
from testsrunner import TestRunnerBase


test_suite_name = 'cmor'

workdir = os.getcwd()

runner = TestRunnerBase(test_suite_name)
ret_code = runner.run(workdir)
sys.exit(ret_code)
