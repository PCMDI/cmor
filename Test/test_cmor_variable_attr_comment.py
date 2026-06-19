import json
import cmor
import unittest
import numpy
from pathlib import Path
from base_CMIP6_CV import BaseCVsTest

from netCDF4 import Dataset


class TestVariableAttributeComment(BaseCVsTest):

    def test_variable_comment_from_user(self):
        user_comment = "Comment from the user"
        table_comment = "Temperature of the lower boundary of the atmosphere"
        table = "CMIP6_Amon.json"

        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table(table)

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

        varid = cmor.variable('ts', 'K', axis_ids, comment=user_comment)

        _ = cmor.write(varid, [273, 273, 273], ntimes_passed=1,
                       time_vals=[15], time_bnds=[10, 20])
        filename = cmor.close(varid, file_name=True)
        self.assertEqual(cmor.close(), 0)

        ds = Dataset(filename)
        var = ds.variables["ts"]
        var_comment = var.getncattr("comment")

        expected_comment = f"{user_comment}, {table} comment: {table_comment}"
        self.assertEqual(var_comment, expected_comment)

        ds.close()


if __name__ == '__main__':
    unittest.main()
