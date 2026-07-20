import json
import tempfile
import unittest
from pathlib import Path

import cmor
import netCDF4
import numpy


REPO_ROOT = Path(__file__).resolve().parents[1]
TABLES_PATH = REPO_ROOT / "cmip7-cmor-tables" / "tables"

def write_user_input(output_dir, frequency):
    user_input = {
        "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
        "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
        "_cmip7_option": 1,
        "_controlled_vocabulary_file": "../tables-cvs/cmor-cvs.json",
        "activity_id": "CMIP",
        "archive_id": "WCRP",
        "calendar": "360_day",
        "cv_version": "6.2.19.0",
        "experiment_id": "amip",
        "forcing_index": "f3",
        "frequency": frequency,
        "grid_label": "g999",
        "host_collection": "CMIP7",
        "initialization_index": "i1",
        "institution_id": "CCCma",
        "license_id": "CC-BY-4.0",
        "nominal_resolution": "100 km",
        "outpath": str(output_dir),
        "physics_index": "p1",
        "realization_index": "r9",
        "region": "glb",
        "source_id": "ACCESS-ESM1-6",
        "output_path_template": "<activity_id><source_id><experiment_id><member_id><variable_id><branding_suffix><grid_label>",
        "output_file_template": "<variable_id><branding_suffix><frequency><region><grid_label><source_id><experiment_id><variant_label>",
    }
    input_path = output_dir / "input.json"
    input_path.write_text(json.dumps(user_input, indent=2), encoding="utf-8")
    return input_path


class TestStoreWithTime1(unittest.TestCase):

    def setUp(self):
        self.ntime = 4
        self.time_vals = numpy.arange(self.ntime, dtype="d") * 0.25
        self.lat_vals = numpy.array([-60.0, 0.0, 60.0], dtype="d")
        self.lat_bnds = numpy.array([-90.0, -30.0, 30.0, 90.0], dtype="d")
        self.lon_vals = numpy.array([45.0, 135.0, 225.0, 315.0], dtype="d")
        self.lon_bnds = numpy.array([0.0, 90.0, 180.0, 270.0, 360.0], dtype="d")
        self.lev_vals = numpy.array([0.10, 0.25, 0.45, 0.70, 0.95], dtype="d")
        self.lev_bnds = numpy.array([0.0, 0.18, 0.34, 0.58, 0.82, 1.0], dtype="d")
        self.nlev = len(self.lev_vals)
        self.nlat = len(self.lat_vals)
        self.nlon = len(self.lon_vals)

        self.hus_data = numpy.zeros((self.ntime, self.nlev, self.nlat, self.nlon), dtype="f")
        self.ps_data = numpy.zeros((self.ntime, self.nlat, self.nlon), dtype="f")
        for i in range(self.ntime):
            self.hus_data[i, :, :, :] = 0.001 + i
            self.ps_data[i, :, :] = 100000.0 + i

    def _cmor_time1_setup(self, input_path, time_axis_with_values=True):
        cmor.setup(inpath=str(TABLES_PATH), netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json(str(input_path))
        cmor.load_table("CMIP7_atmos.json")

        if time_axis_with_values:
            time_axis = cmor.axis("time1", "days since 2010-01-01", coord_vals=self.time_vals)
        else:
            time_axis = cmor.axis("time1", "days since 2010-01-01")
        lat_axis = cmor.axis("latitude", "degrees_north", coord_vals=self.lat_vals, cell_bounds=self.lat_bnds)
        lon_axis = cmor.axis("longitude", "degrees_east", coord_vals=self.lon_vals, cell_bounds=self.lon_bnds)
        lev_axis = cmor.axis("alternate_hybrid_sigma", "1", coord_vals=self.lev_vals, cell_bounds=self.lev_bnds)
        cmor.zfactor(
            zaxis_id=lev_axis,
            zfactor_name="ap",
            units="Pa",
            axis_ids=[lev_axis],
            zfactor_values=numpy.array([10.0, 20.0, 30.0, 40.0, 50.0], dtype="d"),
            zfactor_bounds=numpy.array([0.0, 15.0, 25.0, 35.0, 45.0, 55.0], dtype="d"),
        )
        cmor.zfactor(
            zaxis_id=lev_axis,
            zfactor_name="b",
            axis_ids=[lev_axis],
            zfactor_values=numpy.array([0.05, 0.15, 0.35, 0.60, 0.85], dtype="d"),
            zfactor_bounds=numpy.array([0.0, 0.10, 0.25, 0.50, 0.75, 1.0], dtype="d"),
        )
        ps_id = cmor.zfactor(
            zaxis_id=lev_axis,
            zfactor_name="ps1",
            units="Pa",
            axis_ids=[time_axis, lat_axis, lon_axis],
        )
        var_id = cmor.variable(
            "hus_tpt-al-hxy-u",
            "1",
            [time_axis, lev_axis, lat_axis, lon_axis],
            missing_value=1.0e20,
        )

        return var_id, ps_id

    def test_store_with_time1_axis_values_does_not_extend_time(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)
            input_path = write_user_input(output_dir, "6hr")
            var_id, ps_id = self._cmor_time1_setup(input_path)

            cmor.write(var_id, self.hus_data)
            cmor.write(ps_id, self.ps_data, store_with=var_id)
            filename = cmor.close(var_id, file_name=True)
            cmor.close()

            with netCDF4.Dataset(filename) as dataset:
                self.assertEqual(len(dataset.dimensions["time"]), self.ntime)
                numpy.testing.assert_allclose(dataset.variables["time"][:], self.time_vals)
                self.assertEqual(dataset.variables["hus"].shape, (self.ntime, self.nlev, self.nlat, self.nlon))
                self.assertEqual(dataset.variables["ps"].shape, (self.ntime, self.nlat, self.nlon))

    def test_store_with_time1_axis_values_supports_chunked_associated_writes(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)
            input_path = write_user_input(output_dir, "6hr")
            var_id, ps_id = self._cmor_time1_setup(input_path)

            cmor.write(var_id, self.hus_data)
            for i in range(self.ntime):
                cmor.write(ps_id, self.ps_data[i:i + 1], ntimes_passed=1, store_with=var_id)
            filename = cmor.close(var_id, file_name=True)
            cmor.close()

            with netCDF4.Dataset(filename) as dataset:
                self.assertEqual(len(dataset.dimensions["time"]), self.ntime)
                numpy.testing.assert_allclose(dataset.variables["time"][:], self.time_vals)
                numpy.testing.assert_allclose(
                    dataset.variables["ps"][:, 0, 0],
                    100000.0 + numpy.arange(self.ntime, dtype="f"),
                )

    def test_store_with_time1_axis_values_supports_chunked_variable_writes(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)
            input_path = write_user_input(output_dir, "6hr")
            var_id, ps_id = self._cmor_time1_setup(input_path)

            for i in range(self.ntime):
                cmor.write(var_id, self.hus_data[i:i + 1], ntimes_passed=1)
            cmor.write(ps_id, self.ps_data, store_with=var_id)
            filename = cmor.close(var_id, file_name=True)
            cmor.close()

            with netCDF4.Dataset(filename) as dataset:
                self.assertEqual(len(dataset.dimensions["time"]), self.ntime)
                numpy.testing.assert_allclose(dataset.variables["time"][:], self.time_vals)
                numpy.testing.assert_allclose(
                    dataset.variables["ps"][:, 0, 0],
                    100000.0 + numpy.arange(self.ntime, dtype="f"),
                )

    def test_store_with_time1_axis_values_supports_chunked_time_val_writes(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)
            input_path = write_user_input(output_dir, "6hr")
            var_id, ps_id = self._cmor_time1_setup(input_path, time_axis_with_values=False)

            for i in range(self.ntime):
                cmor.write(var_id, self.hus_data[i:i + 1], time_vals=self.time_vals[i], ntimes_passed=1)
            cmor.write(ps_id, self.ps_data, time_vals=self.time_vals, store_with=var_id)
            filename = cmor.close(var_id, file_name=True)
            cmor.close()

            with netCDF4.Dataset(filename) as dataset:
                self.assertEqual(len(dataset.dimensions["time"]), self.ntime)
                numpy.testing.assert_allclose(dataset.variables["time"][:], self.time_vals)
                numpy.testing.assert_allclose(
                    dataset.variables["ps"][:, 0, 0],
                    100000.0 + numpy.arange(self.ntime, dtype="f"),
                )

    def test_store_with_time1_without_axis_or_passed_time_values_raises(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)
            input_path = write_user_input(output_dir, "6hr")
            var_id, ps_id = self._cmor_time1_setup(input_path, time_axis_with_values=False)

            for i in range(self.ntime):
                cmor.write(var_id, self.hus_data[i:i + 1], time_vals=self.time_vals[i], ntimes_passed=1)
            with self.assertRaises(cmor.CMORError):
                cmor.write(ps_id, self.ps_data, store_with=var_id)


class TestStoreWithTimeBounds(unittest.TestCase):

    def test_store_with_bounded_time_axis_still_writes_expected_records(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)
            input_path = write_user_input(output_dir, "mon")

            ntime, nlev, nlat, nlon = 2, 5, 3, 4
            time_vals = numpy.array([15.0, 45.0], dtype="d")
            time_bnds = numpy.array([0.0, 30.0, 60.0], dtype="d")
            lat_vals = numpy.array([10.0, 20.0, 30.0], dtype="d")
            lat_bnds = numpy.array([5.0, 15.0, 25.0, 35.0], dtype="d")
            lon_vals = numpy.array([0.0, 90.0, 180.0, 270.0], dtype="d")
            lon_bnds = numpy.array([-45.0, 45.0, 135.0, 225.0, 315.0], dtype="d")
            lev_vals = numpy.array([0.92, 0.72, 0.50, 0.30, 0.10], dtype="d")
            lev_bnds = numpy.array([1.00, 0.83, 0.61, 0.40, 0.20, 0.00], dtype="d")

            cmor.setup(inpath=str(TABLES_PATH), netcdf_file_action=cmor.CMOR_REPLACE)
            cmor.dataset_json(str(input_path))
            cmor.load_table("CMIP7_atmos.json")

            time_axis = cmor.axis("time", "days since 1979-01-01", coord_vals=time_vals, cell_bounds=time_bnds)
            lat_axis = cmor.axis("latitude", "degrees_north", coord_vals=lat_vals, cell_bounds=lat_bnds)
            lon_axis = cmor.axis("longitude", "degrees_east", coord_vals=lon_vals, cell_bounds=lon_bnds)
            lev_axis = cmor.axis("standard_hybrid_sigma", "1", coord_vals=lev_vals, cell_bounds=lev_bnds)
            cmor.zfactor(
                zaxis_id=lev_axis,
                zfactor_name="a",
                axis_ids=[lev_axis],
                zfactor_values=numpy.array([0.12, 0.22, 0.30, 0.20, 0.10], dtype="d"),
                zfactor_bounds=numpy.array([0.06, 0.18, 0.26, 0.25, 0.15, 0.00], dtype="d"),
            )
            cmor.zfactor(
                zaxis_id=lev_axis,
                zfactor_name="b",
                axis_ids=[lev_axis],
                zfactor_values=numpy.array([0.80, 0.50, 0.20, 0.10, 0.00], dtype="d"),
                zfactor_bounds=numpy.array([0.94, 0.65, 0.35, 0.15, 0.05, 0.00], dtype="d"),
            )
            cmor.zfactor(zaxis_id=lev_axis, zfactor_name="p0", units="Pa", zfactor_values=100000.0)
            ps_id = cmor.zfactor(
                zaxis_id=lev_axis,
                zfactor_name="ps",
                units="Pa",
                axis_ids=[time_axis, lat_axis, lon_axis],
            )
            var_id = cmor.variable(
                "cl_tavg-al-hxy-u",
                "%",
                [time_axis, lev_axis, lat_axis, lon_axis],
                missing_value=1.0e20,
            )

            data = numpy.arange(ntime * nlev * nlat * nlon, dtype="f").reshape(ntime, nlev, nlat, nlon)
            ps_data = (97000.0 + numpy.arange(ntime * nlat * nlon, dtype="f")).reshape(ntime, nlat, nlon)

            cmor.write(var_id, data)
            cmor.write(ps_id, ps_data, store_with=var_id)
            filename = cmor.close(var_id, file_name=True)
            cmor.close()

            with netCDF4.Dataset(filename) as dataset:
                self.assertEqual(len(dataset.dimensions["time"]), ntime)
                numpy.testing.assert_allclose(dataset.variables["time"][:], time_vals)
                self.assertEqual(dataset.variables["cl"].shape, (ntime, nlev, nlat, nlon))
                self.assertEqual(dataset.variables["ps"].shape, (ntime, nlat, nlon))


if __name__ == "__main__":
    unittest.main()
