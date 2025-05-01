import cmor
import unittest
import numpy
from netCDF4 import Dataset
from test_python_common import *


class TestCRS(unittest.TestCase):

    def make_crs_file(self, crs_wkt=None):
        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json("Test/CMOR_input_example.json")

        grid_table = cmor.load_table("Tables/CMIP6_grids.json")
        lmon_table = cmor.load_table("Tables/CMIP6_Lmon.json")

        cmor.set_table(grid_table)

        x, y, lon_coords, lat_coords, lon_vertices, lat_vertices \
            = gen_irreg_grid(lon, lat)

        axes = [
                {
                    'table_entry': 'y',
                    'units': 'm',
                    'coord_vals': y
                },
                {
                    'table_entry': 'x',
                    'units': 'm',
                    'coord_vals': x
                }
            ]

        axis_ids = [cmor.axis(**axis) for axis in axes]

        grid_id = cmor.grid(axis_ids=axis_ids,
                            latitude=lat_coords,
                            longitude=lon_coords,
                            latitude_vertices=lat_vertices,
                            longitude_vertices=lon_vertices)
        axis_ids.append(grid_id)

        mapnm = 'lambert_conformal_conic'
        params = ["standard_parallel1",
                  "longitude_of_central_meridian",
                  "latitude_of_projection_origin",
                  "false_easting", "false_northing",
                  "standard_parallel2"]
        punits = ["", "", "", "", "", ""]
        pvalues = [-20., 175., 13., 8., 0., 20.]
        cmor.set_crs(grid_id=grid_id,
                     mapping_name=mapnm,
                     parameter_names=params,
                     parameter_values=pvalues,
                     parameter_units=punits,
                     crs_wkt=crs_wkt)

        cmor.set_table(lmon_table)

        axis_ids.append(cmor.axis(table_entry='time',
                            units='months since 1980'))
        pass_axes = [axis_ids[3], axis_ids[2]]

        ivar = cmor.variable(table_entry='baresoilFrac',
                             units='',
                             axis_ids=pass_axes,
                             history='no history',
                             comment='no future'
                             )

        ntimes = 2
        for i in range(ntimes):
            data2d = read_2d_input_files(i, varin2d[0], lat, lon) * 1.E-6
            cmor.write(ivar, data2d, 1, time_vals=Time[i],
                    time_bnds=bnds_time[2 * i:2 * i + 2])

        filename = cmor.close(ivar, file_name=True)

        return filename


    def test_crs_without_crs_wkt(self):
        ds = Dataset(self.make_crs_file())

        self.assertTrue('crs' in ds.variables)
        attrs = ds.variables['crs'].ncattrs()
        test_attrs = {
            'grid_mapping_name': 'lambert_conformal_conic',
            'standard_parallel': numpy.asarray([-20., 20.]),
            'longitude_of_central_meridian': 175.0,
            'latitude_of_projection_origin': 13.0,
            'false_easting': 8.0,
            'false_northing': 0.0
        }

        for attr, val in test_attrs.items():
            self.assertTrue(attr in attrs)
            attr_val = ds.variables['crs'].getncattr(attr)
            if attr == 'standard_parallel':
                self.assertTrue(numpy.array_equal(val, attr_val))
            else:
                self.assertEqual(val, attr_val)

        self.assertTrue('crs_wkt' not in attrs)
        
        ds.close()


    def test_crs_with_crs_wkt(self):
        crs_wkt = "Text for the CRS WKT"
        ds = Dataset(self.make_crs_file(crs_wkt))

        self.assertTrue('crs' in ds.variables)
        attrs = ds.variables['crs'].ncattrs()
        test_attrs = {
            'grid_mapping_name': 'lambert_conformal_conic',
            'standard_parallel': numpy.asarray([-20., 20.]),
            'longitude_of_central_meridian': 175.0,
            'latitude_of_projection_origin': 13.0,
            'false_easting': 8.0,
            'false_northing': 0.0,
            'crs_wkt': crs_wkt
        }

        for attr, val in test_attrs.items():
            self.assertTrue(attr in attrs)
            attr_val = ds.variables['crs'].getncattr(attr)
            if attr == 'standard_parallel':
                self.assertTrue(numpy.array_equal(val, attr_val))
            else:
                self.assertEqual(val, attr_val)
        
        ds.close()


if __name__ == '__main__':
    unittest.main()
