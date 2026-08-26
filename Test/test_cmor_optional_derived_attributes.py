import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path

import cmor
import numpy
from netCDF4 import Dataset


CV_PATH = Path("Tables/CMIP6_CV.json")
INPUT_PATH = Path("Test/CMOR_input_example.json")
DERIVED_ATTRIBUTES = ("source", "experiment", "institution")


class TestOptionalDerivedAttributes(unittest.TestCase):
    def setUp(self):
        self.tmpdir = Path(tempfile.mkdtemp(dir=str(Path("Test").resolve())))
        self.cv_path = self.tmpdir / "CMIP6_CV.json"
        self.input_path = self.tmpdir / "input.json"
        self.output_path = self.tmpdir / "output"
        self.output_path.mkdir()

        with CV_PATH.open() as cv_file:
            self.cv = json.load(cv_file)
        required = self.cv["CV"]["required_global_attributes"]
        self.cv["CV"]["required_global_attributes"] = [
            name for name in required if name not in DERIVED_ATTRIBUTES
        ]
        with self.cv_path.open("w") as cv_file:
            json.dump(self.cv, cv_file)

        with INPUT_PATH.open() as input_file:
            self.user_input = json.load(input_file)
        self.user_input["_controlled_vocabulary_file"] = os.path.relpath(
            self.cv_path, Path("Tables").resolve()
        )
        self.user_input["outpath"] = str(self.output_path)
        self.user_input["frequency"] = "mon"

    def tearDown(self):
        cmor.close()
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def _create_output(self, values):
        user_input = self.user_input.copy()
        for attribute in DERIVED_ATTRIBUTES:
            if attribute in values:
                user_input[attribute] = values[attribute]
            else:
                user_input.pop(attribute, None)
        with self.input_path.open("w") as input_file:
            json.dump(user_input, input_file)

        cmor.setup(
            inpath="Tables",
            netcdf_file_action=cmor.CMOR_REPLACE,
            create_subdirectories=1,
        )
        self.assertEqual(cmor.dataset_json(str(self.input_path)), 0)
        cmor.load_table("CMIP6_Omon.json")
        time = cmor.axis(
            table_entry="time",
            units="days since 2010-01-01",
            coord_vals=numpy.array([15.0, 45.0]),
            cell_bounds=numpy.array([0.0, 30.0, 60.0]),
        )
        variable = cmor.variable("thetaoga", units="deg_C", axis_ids=[time])
        self.assertEqual(cmor.write(variable, numpy.array([1.0, 2.0])), 0)
        filename = cmor.close(variable, file_name=True)
        self.assertEqual(cmor.close(), 0)
        return filename

    def test_user_values_override_optional_cv_defaults(self):
        expected = {
            "source": "User supplied source description",
            "experiment": "User supplied experiment description",
            "institution": "User supplied institution description",
        }
        filename = self._create_output(expected)

        with Dataset(filename) as dataset:
            for attribute, value in expected.items():
                self.assertEqual(dataset.getncattr(attribute), value)

    def test_ids_supply_defaults_when_optional_values_are_absent(self):
        filename = self._create_output({})
        cv = self.cv["CV"]
        expected = {
            "source": cv["source_id"][self.user_input["source_id"]]["source"],
            "experiment": cv["experiment_id"][
                self.user_input["experiment_id"]
            ]["experiment"],
            "institution": cv["institution_id"][
                self.user_input["institution_id"]
            ],
        }

        with Dataset(filename) as dataset:
            for attribute, value in expected.items():
                self.assertEqual(dataset.getncattr(attribute), value)

    def test_user_values_do_not_override_required_source_and_institution(self):
        required = self.cv["CV"]["required_global_attributes"]
        required.extend(("source", "institution"))
        with self.cv_path.open("w") as cv_file:
            json.dump(self.cv, cv_file)

        user_values = {
            "source": "User supplied source description",
            "institution": "User supplied institution description",
        }
        filename = self._create_output(user_values)

        cv = self.cv["CV"]
        expected = {
            "source": cv["source_id"][self.user_input["source_id"]]["source"],
            "institution": cv["institution_id"][
                self.user_input["institution_id"]
            ],
        }

        with Dataset(filename) as dataset:
            for attribute, value in expected.items():
                self.assertEqual(dataset.getncattr(attribute), value)

    def test_user_value_does_not_override_required_experiment(self):
        required = self.cv["CV"]["required_global_attributes"]
        required.append("experiment")
        with self.cv_path.open("w") as cv_file:
            json.dump(self.cv, cv_file)

        with self.assertRaises(cmor.CMORError):
            self._create_output(
                {"experiment": "User supplied experiment description"}
            )

        expected = self.cv["CV"]["experiment_id"][
            self.user_input["experiment_id"]
        ]["experiment"]
        self.assertEqual(cmor.get_cur_dataset_attribute("experiment"), expected)


if __name__ == "__main__":
    unittest.main()
