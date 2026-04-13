import json
import cmor
import unittest
import numpy
from pathlib import Path

from netCDF4 import Dataset

CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = "TestTables/CMIP7_CV.json"

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
    "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
    "_cmip7_option": 1,
    "_controlled_vocabulary_file": CV_PATH,
    "activity_id": "CMIP",
    "branch_time_in_child": 30.0,
    "branch_time_in_parent": 10800.0,
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "drs_specs": "MIP-DRS7",
    "experiment_id": "historical",
    "forcing_index": "f30",
    "grid_label": "gn",
    "initialization_index": "i000001d",
    "institution_id": "PCMDI",
    "license_id": "CC BY 4.0",
    "nominal_resolution": "250 km",
    "outpath": ".",
    "parent_mip_era": "CMIP7",
    "parent_time_units": "days since 1850-01-01",
    "parent_activity_id": "CMIP",
    "parent_source_id": "PCMDI-test-1-0",
    "parent_experiment_id": "piControl",
    "parent_variant_label": "r1i1p1f3",
    "physics_index": "p1",
    "realization_index": "r009",
    "source_id": "PCMDI-test-1-0",
    "tracking_prefix": "hdl:21.14100",
    "host_collection": "CMIP7",
    "frequency": "mon",
    "region": "glb",
    "archive_id": "WCRP",
    "output_path_template": "<activity_id><source_id><experiment_id><member_id><variable_id><branding_suffix><grid_label>",
    "output_file_template": "<variable_id><branding_suffix><frequency><region><grid_label><source_id><experiment_id><variant_label>",
}


class TestCMIP7WithParentAttributes(unittest.TestCase):
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        self.input_json = Path("Test/input_parent_attrs.json")

        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH, netcdf_file_action=cmor.CMOR_REPLACE)

        # Define dataset using USER_INPUT
        with open(self.input_json, "w") as input_file_handle:
            json.dump(USER_INPUT, input_file_handle, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json(str(self.input_json))
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

    def tearDown(self):
        self.input_json.unlink(missing_ok=True)

    def test_cmip7_with_parent_attributes(self):
        data = [27] * (2 * 3 * 4)
        tos = numpy.array(data)
        tos.shape = (2, 3, 4)
        lat = numpy.array([10, 20, 30])
        lat_bnds = numpy.array([5, 15, 25, 35])
        lon = numpy.array([0, 90, 180, 270])
        lon_bnds = numpy.array([-45, 45,
                                135,
                                225,
                                315
                                ])
        time = numpy.array([15.5, 45])
        time_bnds = numpy.array([0, 31, 60])
        cmor.load_table("CMIP7_ocean.json")
        cmorlat = cmor.axis("latitude",
                            coord_vals=lat,
                            cell_bounds=lat_bnds,
                            units="degrees_north")
        cmorlon = cmor.axis("longitude",
                            coord_vals=lon,
                            cell_bounds=lon_bnds,
                            units="degrees_east")
        cmortime = cmor.axis("time",
                             coord_vals=time,
                             cell_bounds=time_bnds,
                             units="days since 2018")
        axes = [cmortime, cmorlat, cmorlon]
        cmortos = cmor.variable("tos_tavg-u-hxy-sea", "degC", axes)
        self.assertEqual(cmor.write(cmortos, tos), 0)
        filename = cmor.close(cmortos, file_name=True)
        self.assertEqual(cmor.close(), 0)

        ds = Dataset(filename)
        attrs = ds.ncattrs()

        parent_attrs = {
            "branch_time_in_child": 30.0,
            "branch_time_in_parent": 10800.0,
            "parent_mip_era": "CMIP7",
            "parent_time_units": "days since 1850-01-01",
            "parent_activity_id": "CMIP",
            "parent_source_id": "PCMDI-test-1-0",
            "parent_experiment_id": "piControl",
            "parent_variant_label": "r1i1p1f3"
        }

        for attr, value in parent_attrs.items():
            self.assertIn(attr, attrs)
            self.assertEqual(value, ds.getncattr(attr))

        ds.close()


if __name__ == '__main__':
    unittest.main()
