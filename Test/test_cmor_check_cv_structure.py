import os
import json
import cmor
import unittest
from base_CMIP6_CV import BaseCVsTest

CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = "cmip7-cmor-tables/tables-cvs/cmor-cvs.json"

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
    "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
    "_cmip7_option": 1,
    "_controlled_vocabulary_file": None,
    "activity_id": "CMIP",
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "experiment_id": "amip",
    "forcing_index": "f3",
    "grid_label": "g999",
    "initialization_index": "i1",
    "institution_id": "CCCma",
    "license_id": "CC-BY-4.0",
    "nominal_resolution": "100 km",
    "outpath": ".",
    "physics_index": "p1",
    "realization_index": "r9",
    "source_id": "DUMMY-MODEL",
    "host_collection": "CMIP7",
    "frequency": "mon",
    "region": "glb",
    "archive_id": "WCRP"
}


class TestCheckCVStructure(BaseCVsTest):

    def test_check_for_cv_json_key(self):

        test_name = "check_for_cv_json_key"
        cv_path = f"TestTables/CMIP7_CV_{test_name}.json"
        input_path = f"Test/input_{test_name}.json"

        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            no_cv_key = cv["CV"]
            with open(cv_path, "w") as cv_outfile:
                json.dump(no_cv_key, cv_outfile, sort_keys=True, indent=4)

        with open(input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        error_flag = cmor.dataset_json(input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        with self.assertRaises(cmor.CMORError):
            cmor.load_table("CMIP7_ocean.json")

        self.assertCV("CV section was not found in table: ")

        os.remove(cv_path)
        os.remove(input_path)

    def test_check_cv_attribute_values(self):

        test_name = "check_cv_attribute_values"
        cv_path = f"TestTables/CMIP7_CV_{test_name}.json"
        input_path = f"Test/input_{test_name}.json"

        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["branding_suffix"] = ["template"]
            cv["CV"]["mip_era"] = {"mip_era": "CMIP7"}
            cv["CV"]["nominal_resolution"] = "10000 km"
            cv["CV"]["frequency"] = ["1hr", "3hr", "day", "mon"]
            cv["CV"]["license"] = "PCMDI"
            with open(cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        with open(input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        error_flag = cmor.dataset_json(input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        cmor.load_table("CMIP7_ocean.json")

        self.assertCV(
            "must be a string",
            "Warning: Attribute \"branding_suffix\""
        )

        self.assertCV(
            "must be a string or an array",
            "Warning: Attribute \"mip_era\""
        )

        self.assertCV(
            "must be an array or object",
            "Warning: Attribute \"nominal_resolution\""
        )

        self.assertCV(
            "must be an object",
            "Warning: Attribute \"frequency\""
        )

        self.assertCV(
            "must be an array or object",
            "Warning: Attribute \"license\""
        )

        os.remove(cv_path)
        os.remove(input_path)

    def test_check_cv_single_value_pairs(self):

        test_name = "check_cv_single_value_pairs"
        cv_path = f"TestTables/CMIP7_CV_{test_name}.json"
        input_path = f"Test/input_{test_name}.json"

        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["institution_id"]["CCCma"] = {"Toronto": "Toronto, ON"}
            with open(cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        with open(input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        error_flag = cmor.dataset_json(input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        cmor.load_table("CMIP7_ocean.json")

        self.assertCV(
            "in attribute \"institution_id\" cannot be an object",
            "Value for \"CCCma\""
        )

        os.remove(cv_path)
        os.remove(input_path)

    def test_check_cv_array_values(self):

        test_name = "check_cv_array_values"
        cv_path = f"TestTables/CMIP7_CV_{test_name}.json"
        input_path = f"Test/input_{test_name}.json"

        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["source_type"] = [
                "AER",
                "AGCM",
                {"ISM": "ice-sheet model that includes ice-flow"}
                ]
            with open(cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        with open(input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        error_flag = cmor.dataset_json(input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        cmor.load_table("CMIP7_ocean.json")

        self.assertCV(
            "has elements in its array that are not strings",
            "Attribute \"source_type\""
        )

        os.remove(cv_path)
        os.remove(input_path)

    def test_check_cv_nested_objects(self):

        test_name = "check_cv_nested_objects"
        cv_path = f"TestTables/CMIP7_CV_{test_name}.json"
        input_path = f"Test/input_{test_name}.json"

        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["nested_attr"] = {}
            cv["CV"]["nested_attr"]["nested_value"] = {
                "sub_attr1": "value",
                "sub_attr2": ["1", "2"],
                "sub_attr3": ["1", ["2"], "3"],
                "sub_attr4": {"key": "value"},
                "sub_attr5": {"key": ["value"]}
            }

            with open(cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        with open(input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        error_flag = cmor.dataset_json(input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        cmor.load_table("CMIP7_ocean.json")

        self.assertCV(
            "has elements in its array that are not strings",
            "Attribute \"sub_attr3\""
        )

        self.assertCV(
            "in attribute \"sub_attr5\" cannot be an array",
            "Value for \"key\""
        )

        os.remove(cv_path)
        os.remove(input_path)


if __name__ == '__main__':
    unittest.main()
