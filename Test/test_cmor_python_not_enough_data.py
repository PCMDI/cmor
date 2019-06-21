import cmor
import numpy
import unittest
import sys
import os
import tempfile


class TestCase(unittest.TestCase):

    # This test demonstrates an exception that gets thrown when
    # ntimes_passed is greater than the number of times passed in data
    def testNotEnoughData(self):

        cmor.setup(inpath='Tables',
                netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json("Test/CMOR_input_example.json")

        table = 'CMIP6_Omon.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time',
                'units': 'days since 1850-01-01 00:00:00',
                'coord_vals': [15.5, 45, ],
                'cell_bounds':[[0, 31], [31, 62]]
                },
                {'table_entry': 'depth_coord_half',
                'units': 'm',
                'coord_vals': [5000., 3000., 2000., 1000.],
                'cell_bounds': [5000., 3000., 2000., 1000., 0]},
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

        for var, units, value in (('zhalfo', 'm', 274.),):
            values = numpy.ones([len(x['coord_vals']) for x in axes]) * value
            values = values.astype("f")
            varid = cmor.variable(var,
                                units,
                                axis_ids,
                                history='variable history',
                                missing_value=-99
                                )
            try:
                cmor.write(varid, values, ntimes_passed=3)
            except Exception as inst:
                self.assertEqual(str(inst), "not enough data is being passed "
                                            "for the number of times passed")
            else:
                raise Exception("cmor.write did not throw expected exception")

        cmor.close()

if __name__ == '__main__':
    unittest.main()
