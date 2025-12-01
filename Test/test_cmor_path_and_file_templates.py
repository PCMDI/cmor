import json
import cmor
import unittest
import os
import numpy

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
    "archive_id": "WCRP"
}


class TestPathAndFileTemplates(unittest.TestCase):

    def gen_cmor_file(self):
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
        version = cmor.get_cur_dataset_attribute('_version')
        filepath = cmor.close(cmortos, file_name=True)

        # Attributes for reconstructing file path
        attrs = USER_INPUT.copy()
        variant_label = ('{realization_index}{initialization_index}'
                         '{physics_index}{forcing_index}').format(**attrs)
        attrs['mip_era'] = 'CMIP7'
        attrs['table_id'] = 'ocean'
        attrs['variable_id'] = 'tos'
        attrs['version'] = version
        attrs['variant_label'] = variant_label
        attrs['branding_suffix'] = 'tavg-u-hxy-sea'

        return (filepath, attrs)

    def test_default_path_and_file_templates(self):

        test_name = "default_path_and_file_templates"
        test_cv_path = f"TestTables/CMIP7_CV_{test_name}.json"

        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE)

        # Remove "DRS" section from CV to use default
        # directory path and file name templates in CMOR
        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"].pop("DRS")
            with open(test_cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Define dataset using USER_INPUT
        with open("Test/input_drs.json", "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = test_cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_drs.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        filepath, attrs = self.gen_cmor_file()

        predicted_path = ('./{mip_era}/{activity_id}/{institution_id}/'
                          '{source_id}/{experiment_id}/{variant_label}/'
                          '{table_id}/{variable_id}/{grid_label}/{version}'
                          ).format(**attrs)
        predicted_file = ('{variable_id}_{table_id}_{source_id}_'
                          '{experiment_id}_{variant_label}_{grid_label}_'
                          '201801-201802.nc').format(**attrs)
        predicted_filepath = os.path.join(predicted_path, predicted_file)
        self.assertEqual(filepath, predicted_filepath)

        os.remove(filepath)
        os.remove(test_cv_path)

    def test_cv_path_and_file_templates(self):

        test_name = "cv_path_and_file_templates"
        test_cv_path = f"TestTables/CMIP7_CV_{test_name}.json"

        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE)

        # Use "DRS" section from CV
        drs = {}
        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            drs = cv["CV"]["DRS"]
            with open(test_cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Define dataset using USER_INPUT
        with open("Test/input_drs.json", "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = test_cv_path
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_drs.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        filepath, attrs = self.gen_cmor_file()

        predicted_path_template = drs["directory_path_template"].replace("/","")
        predicted_path_template = predicted_path_template.replace("<","{").replace(">","}")
        predicted_path_template = predicted_path_template.replace("}{","}/{")

        predicted_file_template = drs["filename_template"]
        predicted_file_template = predicted_file_template.replace("<","{").replace(">","}")
        predicted_file_template = predicted_file_template.replace("}{","}_{")

        predicted_path = f'./{predicted_path_template.format(**attrs)}'
        predicted_file = f'{predicted_file_template.format(**attrs)}_201801-201802.nc'
        predicted_filepath = os.path.join(predicted_path, predicted_file)

        self.assertEqual(filepath.replace('//','/'), predicted_filepath)

        os.remove(filepath)
        os.remove(test_cv_path)

    def test_user_input_path_and_file_templates(self):

        test_name = "user_input_path_and_file_templates"
        test_cv_path = f"TestTables/CMIP7_CV_{test_name}.json"

        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE)

        # Use "DRS" section from CV
        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            with open(test_cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Use directory path and file name templates from user input
        with open("Test/input_drs.json", "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["_controlled_vocabulary_file"] = test_cv_path
            user_input["output_path_template"] = (
                "<activity_id><source_id><experiment_id>"
                "<frequency><variable_id><branding_suffix>"
            )
            user_input["output_file_template"] = (
                "<region><grid_label><variant_label><version>"
            )
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_drs.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        filepath, attrs = self.gen_cmor_file()

        predicted_path = ('./{activity_id}/{source_id}/{experiment_id}/'
                          '{frequency}/{variable_id}/{branding_suffix}/'
                          ).format(**attrs)
        predicted_file = ('{region}_{grid_label}_{variant_label}_{version}'
                          '_201801-201802.nc').format(**attrs)
        predicted_filepath = os.path.join(predicted_path, predicted_file)
        self.assertEqual(filepath, predicted_filepath)

        os.remove(filepath)
        os.remove(test_cv_path)


if __name__ == '__main__':
    unittest.main()
