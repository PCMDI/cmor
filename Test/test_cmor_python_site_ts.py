import cmor
import numpy
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testSiteTS(self):
        try:
            cmor.setup(inpath=self.testdir,
                    netcdf_file_action=cmor.CMOR_REPLACE_3,
                    logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "CMOR_input_example.json"))

            axes = [{'table_entry': 'time1',
                    'units': 'days since 2000-01-01 00:00:00',
                    },
                    {'table_entry': 'site',
                    'units': '',
                    'coord_vals': [0]},
                    {'table_entry': 'hybrid_height',
                    'units': 'm',
                    'coord_vals': numpy.array(range(2), dtype=numpy.float64) + 0.5,
                    'cell_bounds': [[x, x + 1] for x in range(2)],
                    },
                    ]

            values = numpy.array([0.5, 0.5], numpy.float32)

            table = 'CMIP6_CFsubhr.json'
            cmor.load_table(os.path.join(self.tabledir, table))

            axis_ids = list()
            for axis in axes:
                axis_id = cmor.axis(**axis)
                axis_ids.append(axis_id)

            igrid = cmor.grid([axis_ids[1]], [0.], [0.])
            cmor.zfactor(axis_ids[2], 'b', axis_ids=[axis_ids[2]],
                        zfactor_values=numpy.array(range(2), dtype=numpy.float64), 
                        zfactor_bounds=[[x - 0.5, x + 0.5] for x in range(2)])

            cmor.zfactor(axis_ids[2], 'orog', 'm', axis_ids=[igrid],
                        zfactor_values=[0.] )

            ids_for_var = [axis_ids[0], igrid, axis_ids[2]]
            varid = cmor.variable('tnhus',
                                's-1',
                                ids_for_var,
                                history='variable history',
                                missing_value=-99,
                                )

            for time in [x * 1800. / 86400 for x in range(48)]:
                cmor.write(varid, values, time_vals=[time])

            fname = cmor.close(varid, file_name=True)
            print(fname)

            cmor.close()
            self.processLog()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
