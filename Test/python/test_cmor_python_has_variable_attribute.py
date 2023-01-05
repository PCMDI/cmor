# If this example is not executed from the directory containing the
# CMOR code, please first complete the following steps:
#
#   1. In any directory, create 'Tables/' and 'Test/' directories.
#
#   2. Download
#      https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_Omon.json
#      and https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_CV.json
#      to the 'Tables/' directory.
#
#   3. Download
#      https://github.com/PCMDI/cmor/blob/master/Test/
#      test_python_CMIP6_experimentID.json to the 'Test/' directory.
#
# pylint: disable = missing-docstring, invalid-name
"""
Tests for ``cmor.has_variable_attribute``.
"""
import numpy as np
import os
import unittest

import cmor


class TestHasVariableAttribute(unittest.TestCase):
    """
    Tests for ``cmor.has_variable_attribute``.
    """

    def setUp(self):
        self.logfile = 'has_variable_attribute.log'
        cmor.setup(inpath='Tables', logfile=self.logfile)
        cmor.dataset_json('Test/test_python_CMIP6_experimentID.json')
        cmor.load_table('CMIP6_Omon.json')
        coord_vals = np.array([0., 1., 2., 3., 4.])
        cell_bounds = np.array([0., 1., 2., 3., 4., 5.])
        axis_id = cmor.axis(table_entry='time', units='months since 2010',
                            coord_vals=coord_vals, cell_bounds=cell_bounds)
        self.variable_id = cmor.variable(table_entry='masso',
                                         axis_ids=[axis_id], units='kg')
        cmor.set_variable_attribute(self.variable_id, 'valid_attribute','c', 'valid_value')

    def test_has_variable_attribute_with_valid_attribute(self):
        self.assertTrue(cmor.has_variable_attribute(self.variable_id,
                                                    'valid_attribute'))

    # Test an attribute that does not exist
    def test_has_variable_attribute_with_invalid_attribute(self):
        self.assertFalse(cmor.has_variable_attribute(self.variable_id,
                                                     'invalid_attribute'))

    def test_set_variable_attribute_with_string_type(self):
        cmor.set_variable_attribute(self.variable_id, 'myStringAttr','c',
                                    'valid_value')
        self.assertTrue(cmor.has_variable_attribute(self.variable_id,
                                                     'myStringAttr'))

    def test_set_variable_attribute_with_long_type(self):
        cmor.set_variable_attribute(self.variable_id, 'long_attribute','l',
                                    1234)
        self.assertTrue(cmor.has_variable_attribute(self.variable_id,
                                                     'long_attribute'))

    def test_set_variable_attribute_with_int_type(self):
        cmor.set_variable_attribute(self.variable_id, 'int_attribute','i',
                                    1134)
        self.assertTrue(cmor.has_variable_attribute(self.variable_id,
                                                     'int_attribute'))

    def test_set_variable_attribute_with_right_type(self):
        cmor.set_variable_attribute(self.variable_id, 'float_attribute','f',
                                    1.0)
        self.assertTrue(cmor.has_variable_attribute(self.variable_id,
                                                     'float_attribute'))
    # We get a warning
    def test_set_variable_attribute_with_different_type(self):
        cmor.set_variable_attribute(self.variable_id, 'double_attribute','d',
                                    12.34)
        self.assertTrue(cmor.has_variable_attribute(self.variable_id,
                                                     'double_attribute'))
    def tearDown(self):
        cmor.close(self.variable_id)
        if os.path.isfile(self.logfile):
            os.remove(self.logfile)


if __name__ == '__main__':
    unittest.main()
