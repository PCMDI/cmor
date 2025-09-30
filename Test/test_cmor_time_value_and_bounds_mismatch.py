import json
import cmor
import unittest
import os
import numpy
import base_CMIP6_CV

CV_PATH = "TestTables/CMIP7_CV.json"

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "Tables/CMIP6_coordinate.json",
    "_FORMULA_VAR_FILE": "Tables/CMIP6_formula_terms.json",
    "_cmip7_option": 1,
    "_use_strings_for_indexes": 1,
    "_controlled_vocabulary_file": CV_PATH,
    "activity_id": "CMIP",
    "branch_method": "standard",
    "branch_time_in_child": 30.0,
    "branch_time_in_parent": 10800.0,
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "experiment": "1 percent per year increase in CO2",
    "experiment_id": "1pctCO2",
    "forcing_index": "f3",
    "grid": "N96",
    "grid_label": "gn",
    "initialization_index": "i1",
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
    "realization_index": "r9",
    "source_id": "PCMDI-test-1-0",
    "source_type": "AOGCM CHEM BGC",
    "tracking_prefix": "hdl:21.14100",
    "host_collection": "CMIP7",
    "frequency": "xxxxxxxxxxxx",
    "region": "glb",
    "archive_id": "WCRP"
}


class TestTimeBoundsMismatchMakingMonthlyAxis(base_CMIP6_CV.BaseCVsTest):

    def setUp(self):
        super().setUp()

        frequency = "mon"
        self.input_path = f"Test/input_making_{frequency}_axis.json"

        cmor.setup(inpath="TestTables",
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(self.input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["frequency"] = frequency
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json(self.input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")
        
        cmor.load_table("CMIP7_ocean2d.json")

    def tearDown(self):
        os.remove(self.input_path)
        return super().tearDown()
        
    def assertWarningMessage(self, flag):
        warn_msg = (
            "Warning: The values you provided for axis time are different "
            "from those computed from the bounds, which are used for the axis "
            "values instead of the user-provided values."
        )

        with open(self.tmpfile, 'r') as f:
            lines = f.readlines()
            warning_found = any(warn_msg in l for l in lines)
            self.assertEqual(flag, warning_found)

    def test_make_axis_monthly_with_day_units_no_warning(self):
        
        time = numpy.array([15.5, 45.5])
        time_bnds = numpy.array([0, 31, 60])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="days since 2018")

        self.assertWarningMessage(False)

    def test_make_axis_monthly_with_day_units_with_warning(self):
        
        time = numpy.array([15.5, 45])
        time_bnds = numpy.array([0, 31, 60])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="days since 2018")

        self.assertWarningMessage(True)
        
        self.assertCV("45.000000 will be replaced with 45.500000 "
                      "between bounds 31.000000 and 60.000000", 
                      "! The first value found is at index 1: ")
        

    def test_make_axis_monthly_with_month_units(self):
        
        time = numpy.array([0, 1])
        time_bnds = numpy.array([0, 1, 2])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="months since 2018")

        self.assertWarningMessage(True)
        
        self.assertCV("0.000000 will be replaced with 15.000000 "
                      "between bounds 0.000000 and 30.000000", 
                      "! The first value found is at index 0: ")


class TestTimeBoundsMismatchMaking6HourlyAxis(base_CMIP6_CV.BaseCVsTest):

    def setUp(self):
        super().setUp()

        frequency = "6hr"
        self.input_path = f"Test/input_making_{frequency}_axis.json"

        cmor.setup(inpath="TestTables",
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(self.input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["frequency"] = frequency
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json(self.input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")
        
        cmor.load_table("CMIP7_ocean2d.json")

    def tearDown(self):
        os.remove(self.input_path)
        return super().tearDown()
        
    def assertWarningMessage(self, flag):
        warn_msg = (
            "Warning: The values you provided for axis time are different "
            "from those computed from the bounds, which are used for the axis "
            "values instead of the user-provided values."
        )

        with open(self.tmpfile, 'r') as f:
            lines = f.readlines()
            warning_found = any(warn_msg in l for l in lines)
            self.assertEqual(flag, warning_found)

    def test_make_axis_6hourly_with_day_units_no_warning(self):
        
        time = numpy.array([0.125, 0.375])
        time_bnds = numpy.array([0, 0.25, 0.5])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="days since 2018")

        self.assertWarningMessage(False)

    def test_make_axis_6hourly_with_day_units_with_warning(self):
        
        time = numpy.array([0, 0.25])
        time_bnds = numpy.array([0, 0.25, 0.5])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="days since 2018")

        self.assertWarningMessage(True)
        
        self.assertCV("0.000000 will be replaced with 0.125000 "
                      "between bounds 0.000000 and 0.250000", 
                      "! The first value found is at index 0: ")


class TestTimeBoundsMismatchMakingYearlyAxis(base_CMIP6_CV.BaseCVsTest):

    def setUp(self):
        super().setUp()

        frequency = "yr"
        self.input_path = f"Test/input_making_{frequency}_axis.json"

        cmor.setup(inpath="TestTables",
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(self.input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["frequency"] = frequency
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json(self.input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")
        
        cmor.load_table("CMIP7_ocean2d.json")

    def tearDown(self):
        os.remove(self.input_path)
        return super().tearDown()
        
    def assertWarningMessage(self, flag):
        warn_msg = (
            "Warning: The values you provided for axis time are different "
            "from those computed from the bounds, which are used for the axis "
            "values instead of the user-provided values."
        )

        with open(self.tmpfile, 'r') as f:
            lines = f.readlines()
            warning_found = any(warn_msg in l for l in lines)
            self.assertEqual(flag, warning_found)

    def test_make_axis_yearly_with_day_units_no_warning(self):
        
        time = numpy.array([182.5])
        time_bnds = numpy.array([0, 365])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="days since 2018")

        self.assertWarningMessage(False)

    def test_make_axis_yearly_with_day_units_with_warning(self):
        
        time = numpy.array([0])
        time_bnds = numpy.array([0, 365])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="days since 2018")

        self.assertWarningMessage(True)
        
        self.assertCV("0.000000 will be replaced with 182.500000 "
                      "between bounds 0.000000 and 365.000000", 
                      "! The first value found is at index 0: ")

    def test_make_axis_yearly_with_year_units_no_warning(self):
        
        time = numpy.array([0.5])
        time_bnds = numpy.array([0, 1])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="years since 2018")

        self.assertWarningMessage(False)

    def test_make_axis_yearly_with_year_units_with_warning(self):
        
        time = numpy.array([0.5, 1])
        time_bnds = numpy.array([0, 1, 2])

        _ = cmor.axis("time",
                      coord_vals=time,
                      cell_bounds=time_bnds,
                      units="years since 2018")

        self.assertWarningMessage(True)
        
        self.assertCV("360.000000 will be replaced with 540.000000 "
                      "between bounds 360.000000 and 720.000000", 
                      "! The first value found is at index 1: ")


class TestTimeBoundsMismatchTimesPassed(base_CMIP6_CV.BaseCVsTest):
        
    def assertWarningMessage(self, flag):
        warn_msg = (
            "Warning: The values you provided for axis time are different "
            "from those computed from the bounds, which are used for the axis "
            "values instead of the user-provided values."
        )

        with open(self.tmpfile, 'r') as f:
            lines = f.readlines()
            warning_found = any(warn_msg in l for l in lines)
            self.assertEqual(flag, warning_found)

    def test_time_value_bounds_mismatch_times_passed(self):

        test_name = "time_value_bounds_mismatch_times_passed"
        input_path = f"Test/input_{test_name}.json"
        frequency = "mon"

        cmor.setup(inpath="TestTables",
                   netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile=self.tmpfile)

        with open(input_path, "w") as input_file:
            user_input = USER_INPUT.copy()
            user_input["frequency"] = frequency
            json.dump(user_input, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json(input_path)
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

        lat = numpy.array([10, 20, 30])
        lat_bnds = numpy.array([5, 15, 25, 35])
        lon = numpy.array([0, 90, 180, 270])
        lon_bnds = numpy.array([-45, 45, 135, 225, 315])
        time = numpy.array([15.5, 45])
        time_bnds = numpy.array([0, 31, 60])
        tos_shape = (time.shape[0], lat.shape[0], lon.shape[0])
        tos = numpy.full(tos_shape, 27)
        cmor.load_table("CMIP7_ocean2d.json")
        cmorlat = cmor.axis("latitude",
                            coord_vals=lat,
                            cell_bounds=lat_bnds,
                            units="degrees_north")
        cmorlon = cmor.axis("longitude",
                            coord_vals=lon,
                            cell_bounds=lon_bnds,
                            units="degrees_east")
        cmortime = cmor.axis("time",
                             units="days since 2018")
        axes = [cmortime, cmorlat, cmorlon]
        cmortos = cmor.variable("tos_tavg-u-hxy-sea", "degC", axes)

        self.assertWarningMessage(False)

        self.assertEqual(cmor.write(cmortos, tos, ntimes_passed=2,
                                    time_vals=time, time_bnds=time_bnds), 0)

        self.assertWarningMessage(True)
        
        self.assertCV("45.000000 will be replaced with 45.500000 "
                      "between bounds 31.000000 and 60.000000", 
                      "! The first value found is at index 1: ")

        os.remove(input_path)


if __name__ == '__main__':
    unittest.main()
