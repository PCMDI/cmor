import cmor
import unittest
import numpy as np
from netCDF4 import Dataset
from base_CMIP6_CV import BaseCVsTest
from test_python_common import gen_irreg_grid, read_2d_input_files, \
                               Time, bnds_time, varin2d, lat, lon


class TestCRS(BaseCVsTest):

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
        
        if crs_wkt is not None:
            params.append("crs_wkt")
            punits.append("")
            pvalues.append(crs_wkt)

        cmor.set_crs(grid_id=grid_id,
                     mapping_name=mapnm,
                     parameter_names=params,
                     parameter_values=pvalues,
                     parameter_units=punits)

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
            'standard_parallel': np.asarray([-20., 20.]),
            'longitude_of_central_meridian': 175.0,
            'latitude_of_projection_origin': 13.0,
            'false_easting': 8.0,
            'false_northing': 0.0
        }

        for attr, val in test_attrs.items():
            self.assertTrue(attr in attrs)
            attr_val = ds.variables['crs'].getncattr(attr)
            if attr == 'standard_parallel':
                self.assertTrue(np.array_equal(val, attr_val))
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
            'standard_parallel': np.asarray([-20., 20.]),
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
                self.assertTrue(np.array_equal(val, attr_val))
            else:
                self.assertEqual(val, attr_val)
        
        ds.close()


class TestLatLonGridMapping(BaseCVsTest):

    def test_latitude_longitude_grid_mapping(self):
        crs_wkt = \
            """
            GEOGCS[
                "WGS 84",
                DATUM[
                    "WGS_1984",
                    SPHEROID[
                        "WGS 84", 6378137, 298.257223563 ,
                        AUTHORITY["EPSG","7030"]
                    ],
                    AUTHORITY["EPSG", "6326"]
                ],
                PRIMEM["Greenwich", 0, AUTHORITY["EPSG", "8901"]],
                UNIT["degree", 0.0174532925199433, AUTHORITY["EPSG", "9122"]],
                AUTHORITY["EPSG", "4326"]
            ]
            """
        crs_params = {
            'grid_mapping_name': 'latitude_longitude',
            'longitude_of_prime_meridian': np.float64(0.0),
            'semi_major_axis': np.float64(6378137.0),
            'long_name': 'WGS 84',
            'inverse_flattening': np.float64(298.257223563),
            'GeoTransform': '-179.5 0.1 0 74.5 0.1',
            'crs_wkt': crs_wkt
            }
        
        grid_params = {
            'longitude_of_prime_meridian': [np.float64(0.0), "degrees_east"],
            'semi_major_axis': {"value": np.float64(6378137.0), "units": "meters"},
            'long_name': 'WGS 84',
            'inverse_flattening': (np.float64(298.257223563), ""),
            'GeoTransform': '-179.5 0.1 0 74.5 0.1',
            'crs_wkt': crs_wkt
            }

        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json("Test/CMOR_input_example.json")

        grid_table = cmor.load_table("Tables/CMIP6_grids.json")
        lmon_table = cmor.load_table("Tables/CMIP6_Lmon.json")

        cmor.set_table(grid_table)

        nlat = 10
        nlon = 20
        lon_coords = np.arange(-120., -120. + 1.*nlon, 1.) + 0.5
        lon_coords = lon_coords % 360.
        lat_coords = np.arange(60., 60 + 1.*nlat, 1.) + 0.5
        lon_bounds = np.zeros((lon_coords.shape[0], 2))
        lon_bounds[:, 0] = lon_coords - 0.5
        lon_bounds[:, 1] = lon_coords + 0.5
        lat_bounds = np.zeros((lat_coords.shape[0], 2))
        lat_bounds[:, 0] = lat_coords - 0.5
        lat_bounds[:, 1] = lat_coords + 0.5

        lat_grid, lon_grid = np.broadcast_arrays(
            np.expand_dims(lat_coords, 0),
            np.expand_dims(lon_coords, 1)
            )

        axes = [
                {
                    "table_entry": "latitude",
                    "units": "degrees_north",
                    "coord_vals": lat_coords,
                    "cell_bounds": lat_bounds
                },
                {
                    "table_entry": "longitude",
                    "units": "degrees_east",
                    "coord_vals": lon_coords,
                    "cell_bounds": lon_bounds
                }
            ]

        axis_ids = [cmor.axis(**axis) for axis in axes]

        grid_id = cmor.grid(axis_ids=axis_ids,
                            latitude=lat_grid,
                            longitude=lon_grid)
        axis_ids.append(grid_id)
        cmor.set_crs(grid_id=grid_id,
                     mapping_name=crs_params['grid_mapping_name'],
                     parameter_names=grid_params)

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
            data2d = read_2d_input_files(i, varin2d[0], nlat, nlon) * 1.E-6
            cmor.write(ivar, data2d, 1, time_vals=Time[i],
                    time_bnds=bnds_time[2 * i:2 * i + 2])

        filename = cmor.close(ivar, file_name=True)
        ds = Dataset(filename)

        self.assertTrue('crs' in ds.variables)
        attrs = ds.variables['crs'].ncattrs()

        for attr, val in crs_params.items():
            self.assertTrue(attr in attrs)
            attr_val = ds.variables['crs'].getncattr(attr)
            self.assertEqual(val, attr_val)
        
        ds.close()


class TestStandardParallel(BaseCVsTest):

    def test_grid_mapping_without_standard_parallel(self):
        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")

        grid_table = cmor.load_table("Tables/CMIP6_grids.json")

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
        params = ["longitude_of_central_meridian",
                  "latitude_of_projection_origin",
                  "false_easting",
                  "false_northing"]
        punits = ["", "", "", ""]
        pvalues = [175., 13., 8., 0.]

        cmor.set_crs(grid_id=grid_id,
                     mapping_name=mapnm,
                     parameter_names=params,
                     parameter_values=pvalues,
                     parameter_units=punits)

        self.assertCV(
            ("you should consider setting standard_parallel for one value "
             "or standard_parallel1 and standard_parallel2 for two values"),
            ("Warning: Grid mapping attribute standard_parallel "
             "has not been set, ")
        )


class TestPolarStereographicAttributeOptions(BaseCVsTest):

    def setUp(self, *args, **kwargs):

        # Create x and y coordinates and bounds.
        coord_vals = np.arange(-533750.0, 533750.0 + 2500.0,
                               2500.0, np.float32)
        coord_bnds = np.zeros((coord_vals.shape[0], 2), np.float32)

        coord_bnds[:, 0] = coord_vals - 1250.0
        coord_bnds[:, 1] = coord_vals + 1250.0

        # Create longitude and latitude fields for a
        # polar stereographic projection.
        xgrid, ygrid = np.broadcast_arrays(
            np.expand_dims(
                coord_vals, 0), np.expand_dims(
                coord_vals, 1))

        rhogrid = np.sqrt(xgrid**2 + ygrid**2)
        cgrid = 2.0 * np.arctan((0.5 / 6378137.0) * rhogrid)
        self.lat_grid = (180.0 / 3.141592654) * np.arcsin(np.cos(cgrid))
        self.lon_grid = (180.0 / 3.141592654) * np.arctan2(xgrid, -ygrid)

        self.axes = [
                {
                    'table_entry': 'y',
                    'units': 'm',
                    'coord_vals': coord_vals,
                    'cell_bounds': coord_bnds
                },
                {
                    'table_entry': 'x',
                    'units': 'm',
                    'coord_vals': coord_vals,
                    'cell_bounds': coord_bnds
                }
            ]

        return super().setUp(*args, **kwargs)

    def test_polar_stereographic_without_attribute_options(self):

        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")

        grid_table = cmor.load_table("Tables/CMIP6_grids.json")

        cmor.set_table(grid_table)

        axis_ids = [cmor.axis(**axis) for axis in self.axes]

        grid_id = cmor.grid(axis_ids=axis_ids,
                            latitude=self.lat_grid,
                            longitude=self.lon_grid)
        axis_ids.append(grid_id)

        mapnm = 'polar_stereographic'
        param_dict = {
            'latitude_of_projection_origin': [90.0, 'degrees_north'],
            'longitude_of_projection_origin': [135.0, 'degrees_east'],
            'false_northing': [0.0, 'meters'],
            'false_easting': [0.0, 'meters']
        }

        cmor.set_crs(grid_id=grid_id,
                     mapping_name=mapnm,
                     parameter_names=param_dict)

        self.assertCV(
            ("You should consider setting one of the attributes"),
            ("Warning: Grid mapping attributes standard_parallel or "
             "scale_factor_at_projection_origin were not set for "
             f"grid mapping {mapnm}. ")
        )

    def test_polar_stereographic_with_both_attribute_options(self):

        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")

        grid_table = cmor.load_table("Tables/CMIP6_grids.json")

        cmor.set_table(grid_table)

        axis_ids = [cmor.axis(**axis) for axis in self.axes]

        grid_id = cmor.grid(axis_ids=axis_ids,
                            latitude=self.lat_grid,
                            longitude=self.lon_grid)
        axis_ids.append(grid_id)

        mapnm = 'polar_stereographic'
        param_dict = {
            'latitude_of_projection_origin': [90.0, 'degrees_north'],
            'longitude_of_projection_origin': [135.0, 'degrees_east'],
            'standard_parallel': [70.0, 'degrees_north'],
            'scale_factor_at_projection_origin': [1.0, 'm'],
            'false_northing': [0.0, 'meters'],
            'false_easting': [0.0, 'meters']
        }

        cmor.set_crs(grid_id=grid_id,
                     mapping_name=mapnm,
                     parameter_names=param_dict)

        self.assertCV(
            ("You should consider setting only one of the attributes"),
            ("Warning: Both grid mapping attributes standard_parallel and "
             "scale_factor_at_projection_origin were set for "
             f"grid mapping {mapnm}. ")
        )

    def test_polar_stereographic_attribute_with_wrong_units(self):

        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)
        cmor.dataset_json("Test/CMOR_input_example.json")

        grid_table = cmor.load_table("Tables/CMIP6_grids.json")

        cmor.set_table(grid_table)

        axis_ids = [cmor.axis(**axis) for axis in self.axes]

        grid_id = cmor.grid(axis_ids=axis_ids,
                            latitude=self.lat_grid,
                            longitude=self.lon_grid)
        axis_ids.append(grid_id)

        mapnm = 'polar_stereographic'
        param_dict = {
            'latitude_of_projection_origin': [90.0, 'degrees_north'],
            'longitude_of_projection_origin': [135.0, 'degrees_east'],
            'scale_factor_at_projection_origin': [1.0, 'xyz'],
            'false_northing': [0.0, 'meters'],
            'false_easting': [0.0, 'meters']
        }

        with self.assertRaises(cmor.CMORError):
            _ = cmor.set_crs(grid_id=grid_id,
                             mapping_name=mapnm,
                             parameter_names=param_dict)

        self.assertCV(
            "(xyz)",
            "Error: Udunits: analyzing units from cmor "
        )
        self.assertCV(
            "xyz and m",
            "Error: Udunits: Error preparing units "
        )


if __name__ == '__main__':
    unittest.main()
