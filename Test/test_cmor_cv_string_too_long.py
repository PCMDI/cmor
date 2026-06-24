import json
import cmor
import unittest
import os
from collections.abc import Mapping
from pathlib import Path

from netCDF4 import Dataset
from base_CMIP6_CV import BaseCVsTest

DATASET_INFO = {
    "_AXIS_ENTRY_FILE": "Tables/CMIP6_coordinate.json",
    "_FORMULA_VAR_FILE": "Tables/CMIP6_formula_terms.json",
    "_controlled_vocabulary_file": "",
    "activity_id": "CMIP",
    "branch_method": "standard",
    "branch_time_in_child": 30.0,
    "branch_time_in_parent": 10800.0,
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "domain_id": "EUR-50",
    "experiment": "AMIP",
    "experiment_id": "amip",
    "forcing_index": "3",
    "further_info_url": "https://furtherinfo.es-doc.org/CMIP6.MOHC.HadGEM3-GC31-LL.amip.none.r1i1p1f3",
    "grid": "N96",
    "grid_label": "gn",
    "initialization_index": "1",
    "institution": "Met Office Hadley Centre, Fitzroy Road, Exeter, Devon, EX1 3PB, UK",
    "institution_id": "MOHC",
    "license": "CMIP6 model data produced by the Met Office Hadley Centre is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License (https://creativecommons.org/licenses). Consult https://pcmdi.llnl.gov/CMIP6/TermsOfUse for terms of use governing CMIP6 output, including citation requirements and proper acknowledgment. Further information about this data, including some limitations, can be found via the further_info_url (recorded as a global attribute in this file) and at https://ukesm.ac.uk/cmip6. The data producers and data providers make no warranty, either express or implied, including, but not limited to, warranties of merchantability and fitness for a particular purpose. All liabilities arising from the supply of the information (including any liability arising in negligence) are excluded to the fullest extent permitted by law.",
    "mip_era": "CMIP6",
    "nominal_resolution": "250 km",
    "outpath": ".",
    "parent_activity_id": "no parent",
    "physics_index": "1",
    "realization_index": "1",
    "source": "HadGEM3-GC31-LL (2016): \naerosol: UKCA-GLOMAP-mode\natmos: MetUM-HadGEM3-GA7.1 (N96; 192 x 144 longitude/latitude; 85 levels; top level 85 km)\natmosChem: none\nland: JULES-HadGEM3-GL7.1\nlandIce: none\nocean: NEMO-HadGEM3-GO6.0 (eORCA1 tripolar primarily 1 deg with meridional refinement down to 1/3 degree in the tropics; 360 x 330 longitude/latitude; 75 levels; top grid cell 0-1 m)\nocnBgchem: none\nseaIce: CICE-HadGEM3-GSI8 (eORCA1 tripolar primarily 1 deg; 360 x 330 longitude/latitude)",
    "source_id": "HadGEM3-GC31-LL",
    "source_type": "AGCM",
    "sub_experiment": "none",
    "sub_experiment_id": "none",
    "tracking_prefix": "hdl:21.14100",
    "variant_label": "r1i1p1f3"
}


def deep_update(source, overrides):
    """Recursively merges overrides into source dictionary."""
    for key, value in overrides.items():
        if isinstance(value, Mapping):
            # If the value is a dictionary, dive deeper
            source[key] = deep_update(source.get(key, {}), value)
        else:
            # Otherwise, override or add the value
            source[key] = value
    return source


class TestCVStringTooLong(BaseCVsTest):
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        super().setUp()

        self.cv_file = Path("Test/CMIP6_CV_nested_attribute.json")
        self.user_input_file = Path("Test/input_nested_attribute.json")

        # Set up CMOR
        cmor.setup(inpath="Tables", netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile, create_subdirectories=0)

        # Add 'mip_era' that is longer than 1023 characters
        mip_era_too_long = "x" * 1024
        with open("Tables/CMIP6_CV.json", "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["mip_era"] = mip_era_too_long
            with open(self.cv_file, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Define dataset using DATASET_INFO
        with open(self.user_input_file, "w") as input_file:
            user_input = DATASET_INFO.copy()
            user_input["_controlled_vocabulary_file"] = str(self.cv_file)
            json.dump(user_input, input_file, sort_keys=True, indent=4)

    def tearDown(self):
        super().tearDown()

        self.cv_file.unlink()
        self.user_input_file.unlink()

    def setup_cv(self, update_values: dict):

        # Update values in CV file
        with open("Tables/CMIP6_CV.json", "r") as cv_infile:
            cv = json.load(cv_infile)
            deep_update(cv["CV"], update_values)
            with open(self.cv_file, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Define dataset using DATASET_INFO
        with open(self.user_input_file, "w") as input_file:
            user_input = DATASET_INFO.copy()
            user_input["_controlled_vocabulary_file"] = str(self.cv_file)
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json(str(self.user_input_file))
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

    def test_mip_era_too_long(self):
        attr = "mip_era"
        length = 1024
        value = "x" * length
        self.setup_cv({attr: value})

        mip_table = "CMIP6_Omon.json"
        with self.assertRaises(cmor.CMORError):
            _ = cmor.load_table(mip_table)

        self.assertCV(
            f"Attribute \"{attr}\" has value \"{value[:24]}...\" with a length of "
            f"{length} characters, which exceeds the 1023 character limit."
        )

    def test_nominal_resolution_too_long(self):
        attr = "nominal_resolution"
        length = 2000
        value = "x" * length
        self.setup_cv({attr: [value]})

        mip_table = "CMIP6_Omon.json"
        with self.assertRaises(cmor.CMORError):
            _ = cmor.load_table(mip_table)

        self.assertCV(
            f"Attribute \"{attr}\" has value \"{value[:24]}...\" in its array with "
            f"a length of {length} characters, which exceeds the 1023 character limit."
        )

    def test_activity_id_too_long(self):
        attr = "activity_id"
        length = 1080
        value = "x" * length
        self.setup_cv({attr: {value: "Key is too long."}})

        mip_table = "CMIP6_Omon.json"
        with self.assertRaises(cmor.CMORError):
            _ = cmor.load_table(mip_table)

        self.assertCV(
            f"Key value \"{value[:24]}...\" in attribute \"{attr}\" has "
            f"a length of {length} characters, which exceeds the 1023 character limit."
        )

    def test_attribute_name_too_long(self):
        length = 3000
        attr = "x" * length
        self.setup_cv({attr: "Attribute name is too long."})

        mip_table = "CMIP6_Omon.json"
        with self.assertRaises(cmor.CMORError):
            _ = cmor.load_table(mip_table)

        self.assertCV(
            f"Attribute \"{attr[:24]}...\" has a length of {length} characters, "
            "which exceeds the 1023 character limit."
        )


if __name__ == '__main__':
    unittest.main()
