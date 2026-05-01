import json
import shutil
import tempfile
import unittest
from pathlib import Path

import cmor
import numpy
from netCDF4 import Dataset

CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = Path("TestTables/CMIP7_CV.json")

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
    "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
    "_cmip7_option": 1,
    "_controlled_vocabulary_file": None,
    "activity_id": "CMIP",
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "experiment_id": "piControl",
    "forcing_index": "f30",
    "grid_label": "gn",
    "initialization_index": "i000001d",
    "institution_id": "PCMDI",
    "license_id": "CC BY 4.0",
    "nominal_resolution": "250 km",
    "outpath": ".",
    "physics_index": "p1",
    "realization_index": "r009",
    "source_id": "PCMDI-test-1-0",
    "host_collection": "CMIP7",
    "frequency": "mon",
    "region": "glb",
    "archive_id": "WCRP",
}


class TestCVRootStringAttributes(unittest.TestCase):
    def setUp(self):
        self.tmpdir = Path(tempfile.mkdtemp(dir=str(Path("Test").resolve())))
        self.cv_path = self.tmpdir / "CMIP7_CV_root_strings.json"
        self.input_path = self.tmpdir / "input.json"
        self.output_dir = self.tmpdir / "output"
        self.output_dir.mkdir()
        self.cv_relpath = self.cv_path.relative_to(Path.cwd())
        self.input_relpath = self.input_path.relative_to(Path.cwd())
        self.output_relpath = self.output_dir.relative_to(Path.cwd())

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_sets_drs_specs_and_tracking_prefix_from_cv_root_strings(self):
        with CV_PATH.open() as cv_infile:
            cv = json.load(cv_infile)

        cv["CV"]["drs_specs"] = "MIP-DRS7"
        cv["CV"]["tracking_prefix"] = "hdl:21.14100"

        with self.cv_path.open("w") as cv_outfile:
            json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        user_input = USER_INPUT.copy()
        user_input["_controlled_vocabulary_file"] = str(self.cv_relpath)
        user_input["outpath"] = str(self.output_relpath)

        with self.input_path.open("w") as input_file:
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        cmor.setup(inpath=CMIP7_TABLES_PATH, netcdf_file_action=cmor.CMOR_REPLACE)
        error_flag = cmor.dataset_json(str(self.input_relpath))
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        data = numpy.array([27] * (2 * 3 * 4))
        data.shape = (2, 3, 4)
        lat = numpy.array([10, 20, 30])
        lat_bnds = numpy.array([5, 15, 25, 35])
        lon = numpy.array([0, 90, 180, 270])
        lon_bnds = numpy.array([-45, 45, 135, 225, 315])
        time = numpy.array([15.5, 45])
        time_bnds = numpy.array([0, 31, 60])

        cmor.load_table("CMIP7_ocean.json")
        cmorlat = cmor.axis(
            "latitude",
            coord_vals=lat,
            cell_bounds=lat_bnds,
            units="degrees_north",
        )
        cmorlon = cmor.axis(
            "longitude",
            coord_vals=lon,
            cell_bounds=lon_bnds,
            units="degrees_east",
        )
        cmortime = cmor.axis(
            "time",
            coord_vals=time,
            cell_bounds=time_bnds,
            units="days since 2018",
        )
        cmortos = cmor.variable("tos_tavg-u-hxy-sea", "degC", [cmortime, cmorlat, cmorlon])
        self.assertEqual(cmor.write(cmortos, data), 0)

        filename = Path(cmor.close(cmortos, file_name=True))
        self.assertEqual(cmor.close(), 0)

        ds = Dataset(filename)
        self.assertNotIn("tracking_prefix", ds.ncattrs())
        self.assertEqual(ds.getncattr("drs_specs"), "MIP-DRS7")
        self.assertTrue(ds.getncattr("tracking_id").startswith("hdl:21.14100/"))
        ds.close()

        filename.unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
