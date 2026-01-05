import os
import json
import cmor
import unittest
import numpy

from netCDF4 import Dataset
import pyfive

BYTES_4MiB = 4 * 2**20
CMIP7_TABLES_PATH = "cmip7-cmor-tables/tables"
CV_PATH = "TestTables/CMIP7_CV.json"

USER_INPUT = {
    "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
    "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
    "_cmip7_option": 1,
    "_controlled_vocabulary_file": CV_PATH,
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
    "frequency": "day",
    "region": "glb",
    "archive_id": "WCRP",
    "output_path_template": "<activity_id><source_id><experiment_id><member_id><variable_id><branding_suffix><grid_label>",
    "output_file_template": "<variable_id><branding_suffix><frequency><region><grid_label><source_id><experiment_id><variant_label>",
}


class TestChunking(unittest.TestCase):

    def assertChunking(self, filename, msg=None):
        """ Chunking checks from cmip7_repack """
        f = pyfive.File(filename)
        if "time" in f:
            # Check for the time coordinates variable having one chunk
            t = f["time"]
            chunks = t.chunks
            if chunks is not None and t.id.get_num_chunks() > 1:
                # At least two chunks
                message = self._formatMessage(msg,
                    f"FAIL: File {filename!r} time coordinates variable "
                    f"'time' has {t.id.get_num_chunks()} chunks "
                    "(expected 1 chunk or contiguous)"
                )
                raise self.failureException(message)

            # Check for the time bounds variable having one chunk
            if "bounds" in t.attrs:
                bounds = str(numpy.array(t.attrs["bounds"]).astype("U"))
                if bounds in f:
                    b = f[bounds]
                    chunks = b.chunks
                    if chunks is not None and b.id.get_num_chunks() > 1:
                        # At least two chunks
                        message = self._formatMessage(msg,
                            f"FAIL: File {filename!r} time bounds variable "
                            f"{bounds!r} has {b.id.get_num_chunks()} chunks "
                            "(expected 1 chunk or contiguous)"
                        )
                        raise self.failureException(message)

        # Check for the data variable having one chunks of at least ~4MiB
        if "variable_id" in f.attrs:
            variable_id = str(numpy.array(f.attrs["variable_id"]).astype("U"))
            if variable_id in f:
                d = f[variable_id]
                if chunks is not None and d.id.get_num_chunks() > 1:
                    # At least two chunks
                    chunks = d.chunks
                    wordsize = d.dtype.itemsize
                    chunksize = numpy.prod(chunks) * wordsize

                    lee_way = 0
                    if len(chunks) > 1:
                        lee_way = numpy.prod(chunks[1:]) * wordsize

                    if chunksize + lee_way < BYTES_4MiB:
                        message = self._formatMessage(msg,
                            f"FAIL: File {filename!r} data variable "
                            f"{variable_id!r} has uncompressed chunk size "
                            f"{chunksize} B (expected at least "
                            f"{BYTES_4MiB - lee_way} B or 1 chunk "
                            "or contiguous)"
                        )
                        raise self.failureException(message)
        
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        # Set up CMOR
        cmor.setup(inpath=CMIP7_TABLES_PATH, netcdf_file_action=cmor.CMOR_REPLACE)

        # Define dataset using USER_INPUT
        with open("Test/input_cmip7.json", "w") as input_file_handle:
            json.dump(USER_INPUT, input_file_handle, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_cmip7.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")
        
        self.cmor_filepath = ""

    def tearDown(self):
        if os.path.isfile(self.cmor_filepath):
            os.remove(self.cmor_filepath)

    def generate_file(self, num_times, num_lat, num_lon):

        data = [27] * (num_times * num_lat * num_lon)
        pr = numpy.array(data)
        pr.shape = (num_times, num_lat, num_lon)
        lat_bnds = numpy.linspace(-90., 90., num_lat + 1)
        lat = (lat_bnds[:-1] + lat_bnds[1:]) / 2
        lon_bnds = numpy.linspace(0., 180., num_lon + 1)
        lon = (lon_bnds[:-1] + lon_bnds[1:]) / 2

        time = numpy.arange(num_times) + 0.5
        time_bnds = numpy.arange(num_times + 1)

        cmor.load_table("CMIP7_atmos.json")
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
        cmorpr = cmor.variable("pr_tavg-u-hxy-u", "kg m-2 s-1", axes)
        self.assertEqual(cmor.write(cmorpr, pr), 0)
        self.cmor_filepath = cmor.close(cmorpr, file_name=True)
        self.assertEqual(cmor.close(), 0)

    def test_few_small_timesteps(self):
        num_times = 4
        num_lat = 100
        num_lon = 100
        self.generate_file(num_times, num_lat, num_lon)

        self.assertChunking(self.cmor_filepath)

    def test_many_small_timesteps(self):
        num_times = 200
        num_lat = 100
        num_lon = 100
        self.generate_file(num_times, num_lat, num_lon)

        self.assertChunking(self.cmor_filepath)

    def test_large_timesteps(self):
        num_times = 10
        num_lat = 1000
        num_lon = 1000
        self.generate_file(num_times, num_lat, num_lon)

        self.assertChunking(self.cmor_filepath)

    def test_writing_one_timestep_at_a_time(self):
        num_times = 4
        num_lat = 100
        num_lon = 100

        data = [27] * (num_times * num_lat * num_lon)
        pr = numpy.array(data)
        pr.shape = (num_times, num_lat, num_lon)
        lat_bnds = numpy.linspace(-90., 90., num_lat + 1)
        lat = (lat_bnds[:-1] + lat_bnds[1:]) / 2
        lon_bnds = numpy.linspace(0., 180., num_lon + 1)
        lon = (lon_bnds[:-1] + lon_bnds[1:]) / 2

        time = numpy.arange(num_times) + 0.5
        time_bnds = numpy.arange(num_times + 1)

        cmor.load_table("CMIP7_atmos.json")
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
        cmorpr = cmor.variable("pr_tavg-u-hxy-u", "kg m-2 s-1", axes)
        for i in range(num_times):
            self.assertEqual(cmor.write(cmorpr, pr[i,:,:], time_vals=time[i], time_bnds=time_bnds[i:i+2]), 0)
        self.cmor_filepath = cmor.close(cmorpr, file_name=True)
        self.assertEqual(cmor.close(), 0)

        with Dataset(self.cmor_filepath, 'r') as dataset:
            pr_chunks = dataset.variables['pr'].chunking()
            self.assertEqual(pr_chunks, [1, num_lat, num_lon])
            time_chunks = dataset.variables['time'].chunking()
            self.assertEqual(time_chunks, [512])
            time_bnds_chunks = dataset.variables['time_bnds'].chunking()
            self.assertEqual(time_bnds_chunks, [512, 2])


if __name__ == '__main__':
    unittest.main()
