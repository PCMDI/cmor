import cmor
import numpy
import unittest


class TestCMIP6Plus(unittest.TestCase):

    def test_multiple_modeling_realms(self):
        ntimes_passed = 12

        self.assertEqual(
            cmor.setup(inpath='mip-cmor-tables/Tables',
                       netcdf_file_action=cmor.CMOR_REPLACE),
            0)
        self.assertEqual(cmor.dataset_json("Test/CMIP6Plus_user_input.json"),
                         0)

        self.assertEqual(cmor.load_table("MIP_OPmon.json"), 0)

        axes = [
                {
                    'table_entry': 'time',
                    'units': 'months since 2010-1-1',
                    'coord_vals': numpy.arange(0, ntimes_passed, 1),
                    'cell_bounds': numpy.arange(0, ntimes_passed+1, 1)
                },
                {
                    'table_entry': 'latitude',
                    'units': 'degrees_north',
                    'coord_vals': [0],
                    'cell_bounds': [-1, 1]
                },
                {
                    'table_entry': 'longitude',
                    'units': 'degrees_east',
                    'coord_vals': [90],
                    'cell_bounds': [89, 91]
                }
            ]

        axis_ids = [cmor.axis(**axis) for axis in axes]

        ivar = cmor.variable('hfsifrazil2d', units='W m-2', axis_ids=axis_ids)

        data = [280., ] * ntimes_passed

        self.assertEqual(
            cmor.write(ivar, data, ntimes_passed=ntimes_passed),
            0)

        self.assertFalse(cmor.has_cur_dataset_attribute('further_info_url'))

        realms = cmor.get_cur_dataset_attribute('realm')
        self.assertEqual(realms, "ocean seaIce")

        title = cmor.get_cur_dataset_attribute('title')
        self.assertEqual(title,
                         'HadGEM3-GC31-LL output prepared for CMIP6Plus')

        self.assertEqual(cmor.close(), 0)


if __name__ == '__main__':
    unittest.main()
