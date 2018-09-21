import cmor
import numpy
import unittest


class TestCase(unittest.TestCase):

    def testJamieSiteSurface(self):
        try:
            cmor.setup(inpath='Tables',
                    netcdf_file_action=cmor.CMOR_REPLACE_3,
                    create_subdirectories=0)
            cmor.dataset_json("Test/common_user_input.json")

            axes = [{'table_entry': 'time1',
                    'units': 'days since 2000-01-01 00:00:00',
                    },
                    {'table_entry': 'site',
                    'units': '',
                    'coord_vals': [0]},
                    ]

            values = numpy.array([215.], numpy.float32)

            table = 'CMIP6_CFsubhr.json'
            tid1 = cmor.load_table(table)
            site_axis_id = cmor.axis(**axes[1])

            time_axis_id = cmor.axis(**axes[0])

            table2 = 'CMIP6_grids.json'
            tid2 = cmor.load_table(table2)
            gid = cmor.grid([site_axis_id, ], latitude=numpy.array(
                [-20, ]), longitude=numpy.array([150, ]))

            axis_ids = [time_axis_id, gid]
            cmor.set_table(tid1)
            varid = cmor.variable('rlut',
                                'W m-2',
                                axis_ids,
                                history='variable history',
                                missing_value=-99,
                                positive='up'
                                )

            cmor.write(varid, values, time_vals=[15])

            print cmor.close(file_name=True)
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
