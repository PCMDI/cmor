# pylint: disable = missing-docstring, invalid-name
"""
Tests for ``cmor.has_cur_dataset_attribute``.
"""
import os
import unittest

import cmor


class TestHasCurDatasetAttribute(unittest.TestCase):
    """
    Tests for ``cmor.has_cur_dataset_attribute``.
    """

    def setUp(self):
        self.logfile = 'has_cur_dataset_attribute.log'
        cmor.setup(logfile=self.logfile)
        cmor.set_cur_dataset_attribute('valid_attribute', 'valid_value')

    def test_has_cur_dataset_attribute_with_valid_attribute(self):
        self.assertTrue(cmor.has_cur_dataset_attribute('valid_attribute'))

    def test_has_cur_dataset_attribute_with_invalid_attribute(self):
        self.assertFalse(cmor.has_cur_dataset_attribute('invalid_attribute'))

    def tearDown(self):
        cmor.close()
        if os.path.isfile(self.logfile):
            os.remove(self.logfile)


if __name__ == '__main__':
    unittest.main()
