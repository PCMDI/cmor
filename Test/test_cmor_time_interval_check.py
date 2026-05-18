import cmor
import numpy
import unittest
from base_CMIP6_CV import BaseCVsTest


def build_e1hrclimmon_time3():
    """Build a monthly diurnal climatology time axis with month-to-month gaps."""
    points = []
    bounds = []
    for month_index in range(12):
        month_start = month_index * 30.0
        for hour in range(24):
            lower = month_start + hour / 24.0
            upper = month_start + 30.0 + (hour + 1) / 24.0
            points.append((lower + upper) / 2.0)
            bounds.append([lower, upper])
    return numpy.array(points, dtype=numpy.float64), numpy.array(bounds, dtype=numpy.float64)


def run():
    unittest.main()


class TestTimeIntervalCheck(BaseCVsTest):

    def test_interval_too_big_in_axis(self):
        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_Amon.json")

        with self.assertRaises(cmor.CMORError):
            _ = cmor.axis(table_entry='time',
                          units='days since 2000-01-01 00:00:00',
                          coord_vals=[15, 45, 75],
                          cell_bounds=[0, 30, 60, 120])

        # Check for enhanced error message components
        self.assertCV("No frequency attribute provided in dataset configuration.")
        self.assertCV("30 days", "Expected interval between time axis values:")
        self.assertCV("45 days", "Actual interval between time axis values 1 and 2:")
        self.assertCV("Test/CMOR_input_example.json", "Input JSON:")
        self.assertCV("CMIP6_Amon.json", "Table JSON:")

    def test_interval_too_small_for_passed_time(self):
        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_Amon.json")

        itime = cmor.axis(table_entry='time',
                          units='days since 2000-01-01 00:00:00')
        ilat = cmor.axis(table_entry='latitude',
                         units='degrees_north',
                         coord_vals=[0],
                         cell_bounds=[-1, 1])
        ilon = cmor.axis(table_entry='longitude',
                         units='degrees_east',
                         coord_vals=[90],
                         cell_bounds=[89, 91])

        axis_ids = [itime, ilat, ilon]

        varid = cmor.variable('ts', 'K', axis_ids)

        _ = cmor.write(varid, [273, 273, 273], ntimes_passed=1,
                       time_vals=[15], time_bnds=[10, 20])

        with self.assertRaises(cmor.CMORError):
            _ = cmor.write(varid, [273, 273, 273], ntimes_passed=1,
                           time_vals=[35], time_bnds=[30, 40])

        # Check for enhanced error message components
        self.assertCV("Time interval mismatch detected for frequency: 'mon'")
        self.assertCV("30 days", "Expected interval between time axis values:")
        self.assertCV("20 days", "Actual interval between time axis values 0 and 1:")
        self.assertCV("Test/CMOR_input_example.json", "Input JSON:")
        self.assertCV("CMIP6_Amon.json", "Table JSON:")

    def test_interval_warning(self):
        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_Amon.json")

        _ = cmor.axis(table_entry='time',
                      units='days since 2000-01-01 00:00:00',
                      coord_vals=[15, 45, 75],
                      cell_bounds=[0, 30, 60, 100])

        # Check for warning message
        self.assertCV("provided in dataset configuration.", "Warning: No frequency attribute")
        self.assertCV("30 days", "Expected interval between time axis values:")
        self.assertCV("35 days", "Actual interval between time axis values 1 and 2:")
        self.assertCV("Test/CMOR_input_example.json", "Input JSON:")
        self.assertCV("CMIP6_Amon.json", "Table JSON:")

    def test_interval_too_big_in_axis_without_bounds(self):
        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_E1hr.json")

        with self.assertRaises(cmor.CMORError):
            _ = cmor.axis(table_entry='time1',
                          units='minutes since 2000-01-01 00:00:00',
                          coord_vals=[12.6, 90.0])

        self.assertCV("0.041667 days",
                      "Expected interval between time axis values:")
        self.assertCV("0.05375 days (29.0% difference)",
                      "Actual interval between time axis values 0 and 1:")
        self.assertCV("CMIP6_E1hr.json", "Table JSON:")

    def test_interval_check_skipped_for_climatology_time_values(self):
        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_E1hrClimMon.json")

        time_vals, time_bnds = build_e1hrclimmon_time3()
        axis_ids = [
            cmor.axis(table_entry='time3',
                      units='days since 2000-01-01 00:00:00'),
            cmor.axis(table_entry='latitude',
                      units='degrees_north',
                      coord_vals=[0.0],
                      cell_bounds=[-1.0, 1.0]),
            cmor.axis(table_entry='longitude',
                      units='degrees_east',
                      coord_vals=[90.0],
                      cell_bounds=[89.0, 91.0]),
        ]
        varid = cmor.variable('rlut', 'W m-2', axis_ids, positive='up')
        data = numpy.linspace(180.0, 220.0, num=time_vals.size, dtype=numpy.float64)

        filename = None
        try:
            self.assertEqual(
                cmor.write(varid, data, time_vals=time_vals, time_bnds=time_bnds),
                0,
            )
            filename = cmor.close(varid, file_name=True)
            self.delete_files.append(filename)
            self.assertTrue(filename.endswith("-clim.nc"))
        finally:
            cmor.close()

        with open(self.tmpfile) as handle:
            log_text = handle.read()
        self.assertNotIn("Time interval mismatch detected", log_text)
        self.assertNotIn("No frequency attribute provided in dataset configuration.", log_text)


if __name__ == '__main__':
    run()
