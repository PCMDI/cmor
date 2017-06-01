# pylint: disable = missing-docstring, invalid-name
"""
Tests that the time range string in the filename is generated correctly.
"""
import numpy as np
import os
import unittest

import cmor


class TestHasCurDatasetAttribute(unittest.TestCase):
    """
    Tests for ``cmor.has_cur_dataset_attribute``.
    """

    def setUp(self):
        self.logfile = 'filename_time_range.log'
        cmor.setup(inpath='Test',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.logfile)

        cmor.dataset_json("Test/common_user_input.json")

        self.path = None

    # def test_decadal(self):
    #     table = 'Tables/CMIP6_Odec.json'
    #     cmor.load_table(table)
    #     axes = [{'table_entry': 'time',
    #              'units': 'years since 2000-01-01 00:00:00',
    #              'coord_vals': [5, 15],
    #              'cell_bounds': [[0, 10], [10, 20]]
    #              },
    #             {'table_entry': 'latitude',
    #              'units': 'degrees_north',
    #              'coord_vals': [0],
    #              'cell_bounds': [-1, 1]},
    #             {'table_entry': 'longitude',
    #              'units': 'degrees_east',
    #              'coord_vals': [90],
    #              'cell_bounds': [89, 91]},
    #             ]
    #
    #     axis_ids = list()
    #     for axis in axes:
    #         axis_id = cmor.axis(**axis)
    #         axis_ids.append(axis_id)
    #     varid = cmor.variable('hfds', 'W m-2', axis_ids)
    #     cmor.write(varid, [5, 10])
    #     self.path = cmor.close(varid, file_name=True)
    #
    #     self.assertEqual(os.path.basename(self.path),
    #                      'hfds_Odec_PCMDI-test-1-0_piControl-withism_'
    #                      'r11i1p1f1_gr_2000-2019.nc')

    def test_yr(self):
        table = 'Tables/CMIP6_Eyr.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time',
                 'units': 'days since 2000-01-01 00:00:00',
                 'coord_vals': np.array([182.5, 547.5]),
                 'cell_bounds': [[0, 365], [365, 730]]
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
        varid = cmor.variable('baresoilFrac', '%', axis_ids)
        cmor.write(varid, [40, 60])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'baresoilFrac_Eyr_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_2000-2001.nc')

    def test_mon(self):
        table = 'Tables/CMIP6_Amon.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time',
                 'units': 'days since 2000-01-01 00:00:00',
                 'coord_vals': np.array([15, 45]),
                 'cell_bounds': [[0, 30], [30, 60]]
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
        cmor.write(varid, [273, 274])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'ts_Amon_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_200001-200002.nc')

    def test_monclim(self):
        table = 'Tables/CMIP6_Oclim.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time2',
                 'units': 'days since 2000-01-01 00:00:00',
                 'coord_vals': np.array([15, 45]),
                 'cell_bounds': [[0, 30], [30, 60]]
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
        varid = cmor.variable('difmxybo2d', 'm4 s-1', axis_ids)
        cmor.write(varid, [273, 274])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'difmxybo2d_Oclim_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_200001-200002-clim.nc')

    def test_day(self):
        table = 'Tables/CMIP6_Eday.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time',
                 'units': 'hours since 2000-01-01 00:00:00',
                 'coord_vals': np.array([12, 36]),
                 'cell_bounds': [[0, 24], [24, 48]]
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
        cmor.write(varid, [303, 305])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'ts_Eday_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_20000101-20000102.nc')

    def test_6hr(self):
        table = 'Tables/CMIP6_6hrLev.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time1',
                 'units': 'hours since 2000-01-01 00:00:00',
                 'coord_vals': np.array([3, 9]),
                 'cell_bounds': [[0, 6], [6, 12]]
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
        varid = cmor.variable('ps', 'Pa', axis_ids)
        cmor.write(varid, [99300, 100000])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'ps_6hrLev_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_200001010300-200001010900.nc')

    def test_3hr(self):
        table = 'Tables/CMIP6_3hr.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time1',
                 'units': 'hours since 2000-01-01 00:00:00',
                 'coord_vals': np.array([1.5, 4.5]),
                 'cell_bounds': [[0, 3], [3, 6]]
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
        varid = cmor.variable('ps', 'Pa', axis_ids)
        cmor.write(varid, [99300, 100000])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'ps_3hr_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_200001010130-200001010430.nc')

    def test_1hr(self):
        table = 'Tables/CMIP6_E1hr.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time1',
                 'units': 'minutes since 2000-01-01 00:00:00',
                 'coord_vals': np.array([12.5, 37.5]),
                 'cell_bounds': [[0, 25], [25, 50]]
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
        varid = cmor.variable('psl', 'Pa', axis_ids)
        cmor.write(varid, [99300, 100000])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'psl_E1hr_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_200001010012-200001010038.nc')

    def test_1hrclimmon(self):
        table = 'Tables/CMIP6_E1hrClimMon.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time3',
                 'units': 'minutes since 2000-01-01 00:00:00',
                 'coord_vals': np.array([12.5, 37.5]),
                 'cell_bounds': [[0, 25], [25, 50]]
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
        varid = cmor.variable('rlut', 'W m-2', axis_ids, positive='up')
        cmor.write(varid, [186, 190])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'rlut_E1hrClimMon_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_200001010000-200001010050-clim.nc')

    def test_subhr(self):
        table = 'Tables/CMIP6_Esubhr.json'
        cmor.load_table(table)
        axes = [{'table_entry': 'time1',
                 'units': 'seconds since 2000-01-01 00:00:00',
                 'coord_vals': np.array([750, 2250]),
                 'cell_bounds': [[0, 1500], [1500, 3000]]
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
        varid = cmor.variable('pr', 'kg m-2 s-1', axis_ids)
        cmor.write(varid, [0.000005, 0.000006])
        self.path = cmor.close(varid, file_name=True)

        self.assertEqual(os.path.basename(self.path),
                         'pr_Esubhr_PCMDI-test-1-0_piControl-withism_'
                         'r11i1p1f1_gr_20000101001230-20000101003730.nc')

    def tearDown(self):
        if os.path.isfile(self.logfile):
            os.remove(self.logfile)

        if self.path:
            if os.path.exists(self.path):
                os.remove(self.path)


if __name__ == '__main__':
    unittest.main()
