import json
import cmor
import unittest
import numpy

from netCDF4 import Dataset

CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = "TestTables/CMIP7_CV.json"

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
    "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
    "_cmip7_option": 1,
    "_controlled_vocabulary_file": None,
    "activity_id": "CMIP",
    "branch_method": "standard",
    "branch_time_in_child": 30.0,
    "branch_time_in_parent": 10800.0,
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "experiment": "Simulation of the pre-industrial climate",
    "experiment_id": "piControl",
    "forcing_index": "f30",
    "grid": "N96",
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
    "source_type": "AOGCM CHEM BGC",
    "tracking_prefix": "hdl:21.14100",
    "host_collection": "CMIP7",
    "frequency": "mon",
    "region": "glb",
    "archive_id": "WCRP",
    "output_path_template": "<activity_id><source_id><experiment_id><member_id><variable_id><branding_suffix><grid_label>",
    "output_file_template": "<variable_id><branding_suffix><frequency><region><grid_label><source_id><experiment_id><variant_id>[<time_range>].nc",
}


class TestLicenseAttributes(unittest.TestCase):
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH, netcdf_file_action=cmor.CMOR_REPLACE)

        # Add 'license_id' and 'license_url' to required attributes of CV
        updated_cv_path = "TestTables/CMIP7_CV_license_attrs.json"
        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["required_global_attributes"].append("license_id")
            cv["CV"]["required_global_attributes"].append("license_url")
            with open(updated_cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Define dataset using USER_INPUT
        USER_INPUT["_controlled_vocabulary_file"] = updated_cv_path
        with open("Test/input_cmip7.json", "w") as input_file_handle:
            json.dump(USER_INPUT, input_file_handle, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_cmip7.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

    def test_cmip7_with_license_id_and_url(self):
        tos = numpy.array([27, 27, 27, 27,
                           27, 27, 27, 27,
                           27, 27, 27, 27,
                           27, 27, 27, 27,
                           27, 27, 27, 27,
                           27, 27, 27, 27
                           ])
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
        test_attrs = {
            'branding_suffix': 'tavg-u-hxy-sea',
            'temporal_label': 'tavg',
            'vertical_label': 'u',
            'horizontal_label': 'hxy',
            'area_label': 'sea',
            'region': 'glb',
            'frequency': 'mon',
            'archive_id': 'WCRP',
            'mip_era': 'CMIP7',
            'data_specs_version': 'CMIP-7.0.0.0',
            'host_collection': 'CMIP7',
        }

        for attr, val in test_attrs.items():
            self.assertTrue(attr in attrs)
            self.assertEqual(val, ds.getncattr(attr))
        institution_id = USER_INPUT["institution_id"]
        license_id = "CC BY 4.0"
        license_type = "Creative Commons Attribution 4.0 International"
        license_url = "https://creativecommons.org/licenses/by/4.0/"
        license = \
            (f"{license_id}; CMIP7 data produced by {institution_id} is "
             f"licensed under a {license_type} License ({license_url}). "
             "Consult https://pcmdi.llnl.gov/CMIP7/TermsOfUse for terms of "
             "use governing CMIP7 output, including citation requirements and "
             "proper acknowledgment. The data producers and data providers "
             "make no warranty, either express or implied, including, but not "
             "limited to, warranties of merchantability and fitness for a "
             "particular purpose. All liabilities arising from the supply of "
             "the information (including any liability arising in negligence) "
             "are excluded to the fullest extent permitted by law."
             )
        self.assertTrue("license" in attrs)
        self.assertTrue("license_id" in attrs)
        self.assertTrue("license_url" in attrs)
        self.assertTrue("license_type" not in attrs)
        self.assertEqual(license_id, ds.getncattr("license_id"))
        self.assertEqual(license_url, ds.getncattr("license_url"))
        self.assertEqual(license, ds.getncattr("license"))

        ds.close()


if __name__ == '__main__':
    unittest.main()
