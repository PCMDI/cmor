import cmor
import numpy
import unittest


class TestCase(unittest.TestCase):

    def testLonThro360(self):
        try:
            cmor.setup(inpath='Tables',
                    netcdf_file_action=cmor.CMOR_REPLACE_3,
                    create_subdirectories=0)
            cmor.dataset_json("Test/common_user_input.json")


            axes = [{'table_entry': 'time',
                    'units': 'days since 2000-01-01 00:00:00'
                    },
                    {'table_entry': 'latitude',
                    'units': 'degrees',
                    'coord_vals': [0],
                    'cell_bounds': [-0.5, 0.5]},
                    {'table_entry': 'longitude',
                    'units': 'degrees',
                    'coord_vals': [359., 361., 363.],
                    'cell_bounds': [[358, 359.9],
                                    [359.9, 362.],
                                    [362., 364.]]},
                    ]

            values = numpy.array([360., 1., 3.], numpy.float32)

            table = 'CMIP6_Amon.json'
            cmor.load_table(table)

            axis_ids = list()
            for axis in axes:
                axis_ids.append(cmor.axis(**axis))

            table = 'CMIP6_Amon.json'
            cmor.load_table(table)

            varid = cmor.variable('rlut',
                                'W m-2',
                                axis_ids,
                                history='variable history',
                                missing_value=-99,
                                positive='up'
                                )

            cmor.write(varid, values, time_vals=[15], time_bnds=[0, 30])

            cmor.close()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
