import cmor
import numpy
import unittest


class TestCase(unittest.TestCase):

    def testJamie4(self):
        try:
            cmor.setup(inpath='Tables',
                    netcdf_file_action=cmor.CMOR_REPLACE)
            cmor.dataset_json("Test/common_user_input.json")

            table = 'CMIP6_Amon.json'
            cmor.load_table(table)
            axes = [{'table_entry': 'time',
                    'units': 'days since 2000-01-01 00:00:00',
                    },
                    {'table_entry': 'latitude',
                    'units': 'degrees_north',
                    'coord_vals': [0],
                    'cell_bounds': [-1, 1]},
                    {'table_entry': 'longitude',
                    'units': 'degrees_east',
                    'coord_vals': [90],
                    'cell_bounds': [89, 91]},
                    ]

            axis_ids = list()
            for axis in axes:
                axis_id = cmor.axis(**axis)
                axis_ids.append(axis_id)

            for var, units, val in (('ts', 'K', 278), ('ps', 'hPa', 974.2)):
                varid = cmor.variable(var,
                                    units,
                                    axis_ids,
                                    )

                values = numpy.array([val], numpy.float32)
                cmor.write(varid, values, time_vals=[15], time_bnds=[[0, 30]])

            cmor.close()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
