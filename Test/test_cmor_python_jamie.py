import cmor
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testPath(self):
        try:
            cmor.setup(inpath=self.tabledir, netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)

            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            table = 'CMIP6_Amon.json'
            cmor.load_table(table)
            axes = [{'table_entry': 'time',
                    'units': 'days since 2000-01-01 00:00:00',
                    'coord_vals': [15],
                    'cell_bounds': [0, 30]
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
            varid = cmor.variable('ts', 'K', axis_ids)
            cmor.write(varid, [275])
            path = cmor.close(varid, file_name=True)

            print path
            cmor.close()
            self.processLog()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
