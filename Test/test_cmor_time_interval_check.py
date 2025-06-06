import cmor
import unittest
from base_CMIP6_CV import BaseCVsTest


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

        self.assertCV(
            "approximate time axis interval is defined as 2592000.000000 "
            "seconds (30.000000 days), for value 2 we got a difference of "
            "3888000.000000 seconds (45.000000 days), which is 50.000000 % ,"
            " seems too big, check your values"
        )

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

        self.assertCV(
            "approximate time axis interval is defined as 2592000.000000 "
            "seconds (30.000000 days), for value 1 we got a difference of "
            "1728000.000000 seconds (20.000000 days), which is 33.333333 % ,"
            " seems too big, check your values"
        )


if __name__ == '__main__':
    run()
