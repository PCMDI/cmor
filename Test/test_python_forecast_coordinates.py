import json
import numpy as np
import cmor
import unittest
import os

from netCDF4 import Dataset

DATASET_INFO = {
    "_AXIS_ENTRY_FILE": "CMIP6_coordinate_leadtime.json",
    "_FORMULA_VAR_FILE": "Tables/CMIP6_formula_terms.json",
    "_control_vocabulary_file": "Tables/CMIP6_CV.json",
    "activity_id": "CMIP",
    "branch_method": "standard",
    "branch_time_in_child": 30.0,
    "branch_time_in_parent": 10800.0,
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
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


class TestHasForecastCoordinates(unittest.TestCase):
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        # Set up CMOR
        cmor.setup(inpath="TestTables", netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile="cmor.log", create_subdirectories=0)

        # Define dataset using DATASET_INFO
        with open("Test/input_leadtime.json", "w") as input_file_handle:
            json.dump(DATASET_INFO, input_file_handle, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_leadtime.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")


    def test_has_forcast_coordinates(self):
        # load MIP table
        mip_table = "CMIP6_Amon_leadtime.json"
        table_id = cmor.load_table(mip_table)

        # construct axes
        time = cmor.axis(table_entry="time", units="days since 2000-01-01",
                         coord_vals=np.array([30.0, 60.0]),
                         cell_bounds=np.array([15.0, 45.0, 75.0]))
        reftime = cmor.axis(table_entry="reftime1", units="days since 2000-01-01",
                             coord_vals=np.array([10.0]))
        height2m = cmor.axis(table_entry="height2m", units="m", coord_vals=np.array((2.0,)))
        latitude = cmor.axis(table_entry="latitude", units="degrees_north",
                             coord_vals=np.array(range(5)),
                             cell_bounds=np.array(range(6)))
        longitude = cmor.axis(table_entry="longitude", units="degrees_east",
                              coord_vals=np.array(range(5)),
                              cell_bounds=np.array(range(6)))
        axis_ids = [longitude, latitude, time, reftime, height2m]
        tas_var_id = cmor.variable(table_entry="tas", axis_ids=axis_ids, units="K")
        data = np.random.random(50)
        reshaped_data = data.reshape((5, 5, 2, 1, 1))
        # write data
        cmor.write(tas_var_id, reshaped_data)
        filename = cmor.close(tas_var_id, file_name=True)

        ds = Dataset(filename)
        np.testing.assert_array_equal(ds.variables['reftime'][:].data, [10.0])
        np.testing.assert_array_equal(ds.variables['leadtime'][:].data, [20.0, 50.0])
        ds.close()
        os.remove(filename)


class TestWriteSlices(unittest.TestCase):

    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        # Set up CMOR
        cmor.setup(inpath="TestTables", netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile="cmor.log", create_subdirectories=0)

        # Define dataset using DATASET_INFO
        with open("Test/input_leadtime.json", "w") as input_file_handle:
            json.dump(DATASET_INFO, input_file_handle, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_leadtime.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")


    def test_multiple_writes(self):
        # load MIP table
        mip_table = "CMIP6_Amon.json"
        table_id = cmor.load_table(mip_table)

        # construct axes
        time = cmor.axis(table_entry="time", units="days since 2000-01-01")
        height2m = cmor.axis(table_entry="height2m", units="m", coord_vals=np.array((2.0,)))
        latitude = cmor.axis(table_entry="latitude", units="degrees_north",
                             coord_vals=np.array(range(5)),
                             cell_bounds=np.array(range(6)))
        longitude = cmor.axis(table_entry="longitude", units="degrees_east",
                              coord_vals=np.array(range(5)),
                              cell_bounds=np.array(range(6)))
        axis_ids = [longitude, latitude, time, height2m]
        tas_var_id = cmor.variable(table_entry="tas", axis_ids=axis_ids, units="K")
        # write data
        for year in range(2):
            data = np.random.random(25*12)
            reshaped_data = data.reshape((5, 5, 12, 1))
            time_vals = [pt*30 + 360.0 * year + 15.0 for pt in range(12)]
            time_bnds = [360.0 * year ] + [pt*30 + 360.0 * year + 30.0 for pt in range(12)]
            cmor.write(tas_var_id, reshaped_data, time_vals=time_vals, time_bnds=time_bnds)

        filename = cmor.close(tas_var_id, file_name=True)
        ds = Dataset(filename)
        np.testing.assert_array_equal(ds.variables['time'][:].data,  [15.,  45.,  75., 105., 135., 165., 195., 225., 255., 285., 315., 345., 375., 405., 435., 465., 495., 525., 555., 585., 615., 645., 675., 705.])
        ds.close()
        os.remove(filename)


    def test_multiple_writes_with_leadtime(self):
        # load MIP table
        mip_table = "CMIP6_Amon_leadtime.json"
        table_id = cmor.load_table(mip_table)

        # construct axes
        time = cmor.axis(table_entry="time", units="days since 2000-01-01")
        height2m = cmor.axis(table_entry="height2m", units="m", coord_vals=np.array((2.0,)))
        latitude = cmor.axis(table_entry="latitude", units="degrees_north",
                             coord_vals=np.array(range(5)),
                             cell_bounds=np.array(range(6)))
        longitude = cmor.axis(table_entry="longitude", units="degrees_east",
                              coord_vals=np.array(range(5)),
                              cell_bounds=np.array(range(6)))
        reftime = cmor.axis(table_entry="reftime1", units="days since 2000-01-01",
                            coord_vals=np.array([10.0]))
        axis_ids = [longitude, latitude, time, reftime, height2m]

        tas_var_id = cmor.variable(table_entry="tas", axis_ids=axis_ids, units="K")
        # write data
        for year in range(2):
            data = np.random.random(25 * 12)
            reshaped_data = data.reshape((5, 5, 12, 1, 1))
            time_vals = [pt * 30 + 360.0 * year + 15.0 for pt in range(12)]
            time_bnds = [360.0 * year] + [pt * 30 + 360.0 * year + 30.0 for pt in range(12)]
            cmor.write(tas_var_id, reshaped_data, time_vals=time_vals, time_bnds=time_bnds)

        filename = cmor.close(tas_var_id, file_name=True)
        ds = Dataset(filename)
        np.testing.assert_array_equal(ds.variables['time'][:].data,
                                      [15., 45., 75., 105., 135., 165., 195., 225., 255., 285., 315., 345., 375., 405.,
                                       435., 465., 495., 525., 555., 585., 615., 645., 675., 705.])
        ds.close()
        os.remove(filename)


if __name__ == '__main__':
    unittest.main()
