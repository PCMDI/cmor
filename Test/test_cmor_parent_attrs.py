import json
import cmor
import unittest
import numpy
from pathlib import Path

from base_CMIP6_CV import BaseCVsTest
from netCDF4 import Dataset

CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = "TestTables/CMIP7_CV.json"

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
    "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
    "_cmip7_option": 1,
    "_controlled_vocabulary_file": CV_PATH,
    "activity_id": "CMIP",
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "drs_specs": "MIP-DRS7",
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
    "tracking_prefix": "hdl:21.14100",
    "host_collection": "CMIP7",
    "frequency": "mon",
    "region": "glb",
    "archive_id": "WCRP",
    "output_path_template": "<activity_id><source_id><experiment_id><member_id><variable_id><branding_suffix><grid_label>",
    "output_file_template": "<variable_id><branding_suffix><frequency><region><grid_label><source_id><experiment_id><variant_label>",
}


class TestCMIP7WithParentAttributes(BaseCVsTest):
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        super().setUp()
        self.input_json = Path("Test/input_parent_attrs.json")

        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

    def tearDown(self):
        self.input_json.unlink(missing_ok=True)
        super().tearDown()

    def _load_dataset(self, overrides=None, removed_fields=None):
        input_data = USER_INPUT.copy()

        if removed_fields is not None:
            for field in removed_fields:
                input_data.pop(field, None)

        if overrides is not None:
            input_data.update(overrides)

        with open(self.input_json, "w") as input_file_handle:
            json.dump(input_data, input_file_handle, sort_keys=True, indent=4)

        error_flag = cmor.dataset_json(str(self.input_json))
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

    def _write_tos_file(self):
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
        time = numpy.array([15.5, 45.5])
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
        self.delete_files.append(filename)
        self.assertEqual(cmor.close(), 0)
        return filename

    def test_cmip7_without_parent_attributes(self):
        self._load_dataset()
        filename = self._write_tos_file()

        ds = Dataset(filename)
        attrs = ds.ncattrs()

        parent_attrs = [
            "branch_time_in_child",
            "branch_time_in_parent",
            "parent_mip_era",
            "parent_time_units",
            "parent_activity_id",
            "parent_source_id",
            "parent_experiment_id",
            "parent_variant_label"
        ]

        for attr in parent_attrs:
            self.assertNotIn(attr, attrs)

        ds.close()

    def test_cmip7_with_parent_attributes(self):
        self._load_dataset(
            overrides={
                "branch_time_in_child": 30.0,
                "branch_time_in_parent": 10800.0,
                "experiment_id": "historical",
                "parent_mip_era": "CMIP7",
                "parent_time_units": "days since 1850-01-01",
                "parent_source_id": "PCMDI-test-1-0",
                "parent_experiment_id": "piControl",
                "parent_activity_id": "CMIP",
                "parent_variant_label": "r1i1p1f3",
            },
        )
        filename = self._write_tos_file()

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

    def test_cmip7_warns_on_branch_time_in_child_without_parent_experiment(self):
        self._load_dataset(
            overrides={
                "branch_time_in_child": 30.0,
            },
        )
        filename = self._write_tos_file()

        with Dataset(filename) as ds:
            self.assertNotIn("branch_time_in_child", ds.ncattrs())

        self.assertCV(
            'but your dataset has a "branch_time_in_child" defined',
            'Warning: Your experiment does not have a "parent_experiment_id" defined',
            number_of_lines_to_scan=8,
        )
        self.assertCV(
            'The "branch_time_in_child" will be removed from your dataset.',
            'Warning: Your experiment does not have a "parent_experiment_id" defined',
            number_of_lines_to_scan=8,
        )

    def test_cmip7_errors_on_parent_experiment_without_cv_parent(self):
        self._load_dataset(
            overrides={
                "parent_experiment_id": "historical",
            },
        )

        with self.assertRaises(cmor.CMORError):
            self._write_tos_file()

        try:
            cmor.close()
        except BaseException:
            pass

        self.assertCV(
            'Your experiment "piControl" does not have parent experiments.',
            'Error:',
            number_of_lines_to_scan=6,
        )
        self.assertCV(
            'should not define "parent_experiment_id" in their datasets.',
            'Error:',
            number_of_lines_to_scan=6,
        )

    def test_cmip7_errors_when_parent_experiment_required_but_missing(self):
        self._load_dataset(
            overrides={
                "experiment_id": "historical",
            },
        )

        with self.assertRaises(cmor.CMORError):
            self._write_tos_file()

        try:
            cmor.close()
        except BaseException:
            pass

        self.assertCV(
            'Your input attribute "parent_experiment_id" is not defined properly',
            'Error:',
            number_of_lines_to_scan=6,
        )
        self.assertCV(
            'for your experiment "historical"',
            'Error:',
            number_of_lines_to_scan=6,
        )

    def test_cmip7_errors_on_parent_experiment_not_in_cv(self):
        self._load_dataset(
            overrides={
                "branch_time_in_child": 30.0,
                "branch_time_in_parent": 10800.0,
                "experiment_id": "historical",
                "parent_mip_era": "CMIP7",
                "parent_time_units": "days since 1850-01-01",
                "parent_source_id": "PCMDI-test-1-0",
                "parent_experiment_id": "badParent",
                "parent_activity_id": "CMIP",
                "parent_variant_label": "r1i1p1f3",
            },
        )

        with self.assertRaises(cmor.CMORError):
            self._write_tos_file()

        try:
            cmor.close()
        except BaseException:
            pass

        self.assertCV(
            'Your input attribute "parent_experiment_id" with value',
            'Error:',
            number_of_lines_to_scan=8,
        )
        self.assertCV(
            '"badParent" needs to be replaced with value "piControl"',
            'Error:',
            number_of_lines_to_scan=8,
        )

    def test_cmip7_errors_on_incorrect_parent_activity_id(self):
        self._load_dataset(
            overrides={
                "branch_time_in_child": 30.0,
                "branch_time_in_parent": 10800.0,
                "experiment_id": "historical",
                "parent_mip_era": "CMIP7",
                "parent_time_units": "days since 1850-01-01",
                "parent_source_id": "PCMDI-test-1-0",
                "parent_experiment_id": "piControl",
                "parent_activity_id": "BadMIP",
                "parent_variant_label": "r1i1p1f3",
            },
        )

        with self.assertRaises(cmor.CMORError):
            self._write_tos_file()

        try:
            cmor.close()
        except BaseException:
            pass

        self.assertCV(
            'Your input attribute "parent_activity_id" with value',
            'Error:',
            number_of_lines_to_scan=8,
        )
        self.assertCV(
            '"BadMIP" needs to be replaced with value "CMIP"',
            'Error:',
            number_of_lines_to_scan=8,
        )


if __name__ == '__main__':
    unittest.main()
