import cmor
import json
import os
import unittest
from base_CMIP6_CV import BaseCVsTest

CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = "TestTables/CMIP7_CV.json"


def run():
    unittest.main()


class TestFrequencyRequired(BaseCVsTest):
    def _create_cmip7_json(self, frequency=None, extra_fields=None):
        """Helper to create CMIP7-compliant JSON data"""
        json_data = {
            "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
            "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
            "_cmip7_option": 1,
            "_controlled_vocabulary_file": CV_PATH,
            "mip_era": "CMIP7",
            "institution_id": "PCMDI",
            "source_id": "PCMDI-test-1-0",
            "experiment_id": "piControl",
            "activity_id": "CMIP",
            "source_type": "AOGCM",
            "grid_label": "gn",
            "nominal_resolution": "250 km",
            "calendar": "360_day",
            "region": "glb",
            "outpath": ".",
        }

        if frequency is not None:
            json_data["frequency"] = frequency

        if extra_fields:
            json_data.update(extra_fields)

        return json_data

    def test_missing_frequency_with_time_axis_error(self):
        """Frequency should be required for variables with time axis"""
        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        # Create JSON without frequency
        json_data = self._create_cmip7_json(frequency=None)
        json_file = "test_no_frequency_cmip7.json"
        with open(json_file, 'w') as f:
            json.dump(json_data, f)

        cmor.dataset_json(json_file)
        cmor.load_table("CMIP7_atmos.json")

        itime = cmor.axis(table_entry='time', units='days since 2000-01-01')
        ilat = cmor.axis(table_entry='latitude', units='degrees_north',
                         coord_vals=[0], cell_bounds=[-0.5, 0.5])
        ilon = cmor.axis(table_entry='longitude', units='degrees_east',
                         coord_vals=[90], cell_bounds=[89.5, 90.5])

        with self.assertRaises(cmor.CMORError):
            cmor.variable('prra_tavg-u-hxy-is', 'kg m-2 s-1', [itime, ilat, ilon])

        # Use assertCV to check error message in log
        self.assertCV("3.14.1", "Error:", number_of_lines_to_scan=10)
        self.assertCV("frequency' attribute is required", "Error:", number_of_lines_to_scan=10)
        self.assertCV(json_file, "Error:", number_of_lines_to_scan=10)
        os.remove(json_file)

    def test_fx_field_without_frequency_passes(self):
        """Fixed fields should not require frequency"""
        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        # Create JSON without frequency - should be OK for fx fields
        json_data = self._create_cmip7_json(frequency=None)
        json_file = "test_fx_no_freq_cmip7.json"
        with open(json_file, 'w') as f:
            json.dump(json_data, f)

        cmor.dataset_json(json_file)
        cmor.load_table("CMIP7_land.json")

        ilat = cmor.axis(table_entry='latitude', units='degrees_north',
                         coord_vals=[0], cell_bounds=[-0.5, 0.5])
        ilon = cmor.axis(table_entry='longitude', units='degrees_east',
                         coord_vals=[90], cell_bounds=[89.5, 90.5])

        # Should not raise error for fixed field
        var_id = cmor.variable('rootd_ti-u-hxy-lnd', 'kg m-2 s-1', [ilat, ilon])
        self.assertGreaterEqual(var_id, 0)
        os.remove(json_file)

    def test_frequency_mismatch_cmip6(self):
        """CMIP6 tables should warn about frequency mismatch but use table frequency"""
        cmor.setup(inpath='cmip6-cmor-tables/Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        # Create CMIP6 JSON with wrong frequency - will generate warning
        json_data = {
            "mip_era": "CMIP6",
            "institution_id": "PCMDI",
            "source_id": "PCMDI-test-1-0",
            "experiment_id": "piControl",
            "activity_id": "CMIP",
            "source_type": "AOGCM",
            "grid_label": "gn",
            "nominal_resolution": "250 km",
            "frequency": "day",  # Wrong - Amon table expects 'mon'
            "calendar": "360_day",
            "outpath": ".",
        }
        json_file = "test_freq_mismatch_cmip6.json"
        with open(json_file, 'w') as f:
            json.dump(json_data, f)

        cmor.dataset_json(json_file)
        cmor.load_table("CMIP6_Amon.json")

        itime = cmor.axis(table_entry='time', units='days since 2000-01-01')
        ilat = cmor.axis(table_entry='latitude', units='degrees_north',
                         coord_vals=[0], cell_bounds=[-0.5, 0.5])
        ilon = cmor.axis(table_entry='longitude', units='degrees_east',
                         coord_vals=[90], cell_bounds=[89.5, 90.5])

        # Should succeed (no longer raises error)
        var_id = cmor.variable('tas', 'K', [itime, ilat, ilon])
        self.assertGreaterEqual(var_id, 0)

        # Verify warning message was logged (use "Frequency" as trigger to avoid other warnings)
        self.assertCV("Frequency mismatch", "Frequency", number_of_lines_to_scan=6)
        self.assertCV("day", "Frequency", number_of_lines_to_scan=6)
        self.assertCV("mon", "Frequency", number_of_lines_to_scan=6)
        self.assertCV(json_file, "Frequency", number_of_lines_to_scan=6)
        os.remove(json_file)

    def test_hardcoded_frequency_mappings_work(self):
        """Verify hardcoded mappings work without CV frequency section"""
        test_cv_path = "TestTables/CMIP7_CV_no_frequency.json"

        # Create CV without frequency section
        with open(CV_PATH, "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"].pop("frequency", None)  # Remove frequency section
            with open(test_cv_path, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Create JSON with 6hr frequency, using modified CV
        json_data = self._create_cmip7_json(frequency="6hr")
        json_data["_controlled_vocabulary_file"] = test_cv_path
        json_file = "test_hardcoded_freq_cmip7.json"
        with open(json_file, 'w') as f:
            json.dump(json_data, f)

        cmor.setup(inpath=CMIP7_TABLES_PATH,
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)
        cmor.dataset_json(json_file)

        # Load table that should work with 6hr frequency
        cmor.load_table("CMIP7_atmos.json")

        # Should use hardcoded 0.25 day (6 hour) interval
        time_axis = cmor.axis(table_entry='time',
                              units='hours since 2000-01-01',
                              coord_vals=[3, 9, 15, 21, 27],
                              cell_bounds=[0, 6, 12, 18, 24, 30])

        self.assertGreaterEqual(time_axis, 0)

        # Clean up temporary files
        os.remove(json_file)
        os.remove(test_cv_path)


if __name__ == '__main__':
    run()
