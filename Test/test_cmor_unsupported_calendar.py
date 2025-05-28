import cmor
import numpy
import unittest
from base_CMIP6_CV import BaseCVsTest


class TestUnsupportedCalendar(BaseCVsTest):

    def test_unsupported_calendar(self):

        cmor.setup(inpath='Tables',
                    netcdf_file_action=cmor.CMOR_REPLACE,
                    logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.set_cur_dataset_attribute("calendar", "366_day")

        cmor.load_table("CMIP6_Omon.json")

        with self.assertRaises(cmor.CMORError):
            _ = cmor.axis(table_entry="time", units='months since 2010',
                          coord_vals=numpy.array([0, 1, 2, 3, 4.]),
                          cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))

        self.assertCV(("A 366_day calendar is an invalid selection for "
                       "Model Intercomparison Project (MIP) data."))


if __name__ == '__main__':
    unittest.main()
