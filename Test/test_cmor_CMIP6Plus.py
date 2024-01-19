import cmor
import unittest


class TestGrid(unittest.TestCase):

    def test_pass_only_axis_ids(self):
        self.assertEquals(cmor.setup(inpath='mip-cmor-tables/Tables',
                                     netcdf_file_action=cmor.CMOR_REPLACE),
                          0)
        self.assertEquals(cmor.dataset_json("Test/CMIP6Plus_user_input.json"),
                          0)

        self.assertEquals(cmor.load_table("MIP_OPmon.json"), 0)
        itim = cmor.axis(
            table_entry='time',
            units='months since 2010-1-1',
            coord_vals=[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
            cell_bounds=[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])

        ivar = cmor.variable('thetaoga', units='deg_C', axis_ids=[itim, ])

        data = [280., ] * 12  # 12 months worth of data

        self.assertEquals(cmor.write(ivar, data), 0)

        self.assertEquals(cmor.close(), 0)
