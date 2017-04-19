import glob
import os
import unittest


def create_test_suite():
    test_file_strings = glob.glob('Test/test_python_CMIP6_CV*.py')
#    test_file_strings = glob.glob('Test/test_python_CMIP6_CV*further*.py')
    test_file_strings.extend(['Test/test_python_has_cur_dataset_attribute.py',
                              'Test/test_python_has_variable_attribute.py'])
    module_strings = [os.path.splitext(test_file_string)[0].replace('/', '.')
                      for test_file_string in test_file_strings]
    suites = [unittest.defaultTestLoader.loadTestsFromName(module_string)
              for module_string in module_strings]
    test_suite = unittest.TestSuite(suites)
    return test_suite
