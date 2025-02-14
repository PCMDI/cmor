import cmor
import numpy
import unittest
import os
import shutil
import tempfile
from netCDF4 import Dataset


def run():
    unittest.main()


class TestCase(unittest.TestCase):

    def gen_file(self, seed, ntimes, zstd_level,
                 deflate, deflate_level, shuffle,
                 quantize_mode, quantize_nsd, out_dir):

        numpy.random.seed(seed)

        cmor.setup(inpath='Test', netcdf_file_action=cmor.CMOR_REPLACE)

        cmor.dataset_json("Test/CMOR_input_example.json")

        # creates 1 degree grid
        nlat = 18
        nlon = 36
        alats = numpy.arange(180) - 89.5
        bnds_lat = numpy.arange(181) - 90
        alons = numpy.arange(360) + .5
        bnds_lon = numpy.arange(361)
        cmor.load_table("Tables/CMIP6_Amon.json")
        ilat = cmor.axis(
            table_entry='latitude',
            units='degrees_north',
            length=nlat,
            coord_vals=alats,
            cell_bounds=bnds_lat)

        ilon = cmor.axis(
            table_entry='longitude',
            length=nlon,
            units='degrees_east',
            coord_vals=alons,
            cell_bounds=bnds_lon)

        plevs = numpy.array([100000., 92500, 85000, 70000, 60000, 50000, 40000,
                            30000, 25000, 20000, 15000, 10000, 7000, 5000, 3000,
                            2000, 1000, 999, 998, 997, 996, 995, 994, 500, 100])

        itim = cmor.axis(
            table_entry='time',
            units='months since 2030-1-1',
            length=ntimes,
            interval='1 month')

        ilev = cmor.axis(
            table_entry='plev19',
            units='Pa',
            coord_vals=plevs,
            cell_bounds=None)

        var3d_ids = cmor.variable(
            table_entry='ta',
            units='K',
            axis_ids=numpy.array((ilev, ilon, ilat, itim)),
            missing_value=numpy.array([1.0e28, ], dtype=numpy.float32)[0],
            original_name='cloud')

        cmor.set_quantize(var3d_ids, quantize_mode, quantize_nsd)

        use_deflate = 1 if deflate > 0 else 0
        use_shuffle = 1 if shuffle else 0
        cmor.set_deflate(var3d_ids, use_shuffle, use_deflate,
                         deflate_level=deflate_level)
        cmor.set_zstandard(var3d_ids, zstd_level)

        for it in range(ntimes):

            time = numpy.array((it))
            bnds_time = numpy.array((it, it + 1))
            data3d = numpy.random.random((len(plevs), nlon, nlat)) * 30. + 265.
            data3d = data3d.astype('f')
            cmor.write(
                var_id=var3d_ids,
                data=data3d,
                ntimes_passed=1,
                time_vals=time,
                time_bnds=bnds_time)

        nc_path = cmor.close(var3d_ids, file_name=True)

        if deflate:
            dst_file = f'deflate_level_{str(deflate_level)}'
        else:
            dst_file = f'zstd_level_{str(zstd_level)}'
        if shuffle:
            dst_file += '_shuffle'
        dst_file += f'qmode_{str(quantize_mode)}_nsd_{str(quantize_nsd)}.nc'

        dst_path = os.path.join(out_dir, dst_file)

        shutil.copy(nc_path, dst_path)

        return dst_path

    def testZstandardCompression(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            no_compression = self.gen_file(seed, ntimes, 0, True, 0, False, 0, 0, tmp_dir)
            zstd_shuffle = self.gen_file(seed, ntimes, 3, False, 0, True, 0, 0, tmp_dir)

            no_comp_size = os.path.getsize(no_compression)
            zstd_shuffle_size = os.path.getsize(zstd_shuffle)
            print(f'File size without compression: {no_comp_size} bytes')
            print(f'File size with zstandard compression and shuffle: {zstd_shuffle_size} bytes')
            self.assertTrue(zstd_shuffle_size < no_comp_size)

            no_comp_nc = Dataset(no_compression, "r", format="NETCDF4")
            zstd_shuffle_nc = Dataset(zstd_shuffle, "r", format="NETCDF4")

            no_comp_ta = no_comp_nc.variables['ta'][:]
            zstd_shuffle_ta = zstd_shuffle_nc.variables['ta'][:]
            self.assertIsNone(numpy.testing.assert_array_equal(no_comp_ta, zstd_shuffle_ta))

    def testQuantize(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            default = self.gen_file(seed, ntimes, 0, True, 1, False, 0, 0, tmp_dir)
            quantized = self.gen_file(seed, ntimes,  0, True, 1, False, 1, 4, tmp_dir)

            default_size = os.path.getsize(default)
            quantized_size = os.path.getsize(quantized)
            print(f'File size without quantization: {default_size} bytes')
            print(f'File size with quantization: {quantized_size} bytes')
            self.assertTrue(quantized_size < default_size)

            default_nc = Dataset(default, "r", format="NETCDF4")
            quantized_nc = Dataset(quantized, "r", format="NETCDF4")

            default_ta = default_nc.variables['ta']
            quantized_ta = quantized_nc.variables['ta']
            self.assertIsNone(
                numpy.testing.assert_allclose(default_ta[:],
                                              quantized_ta[:],
                                              rtol=1e-4)
            )

            default_attributes = default_ta.ncattrs()

            self.assertTrue('quantization_info' not in default_nc.variables)
            self.assertTrue('quantization' not in default_attributes)
            self.assertTrue('quantization_nsd' not in default_attributes)
            self.assertTrue('quantization_nsb' not in default_attributes)

            quantized_attributes = quantized_ta.ncattrs()

            self.assertTrue('quantization_info' in quantized_nc.variables)
            self.assertTrue('quantization' in quantized_attributes)
            self.assertTrue('quantization_nsd' in quantized_attributes)
            self.assertTrue('quantization_nsb' not in quantized_attributes)

            quantize_attr = quantized_ta.getncattr('quantization')
            quantize_nsd = quantized_ta.getncattr('quantization_nsd')

            self.assertEqual(quantize_attr, 'quantization_info')
            self.assertEqual(quantize_nsd, 4)

            quantize_info = quantized_nc.variables['quantization_info']
            quantize_alg = quantize_info.getncattr('algorithm')
            quantize_impl = quantize_info.getncattr('implementation')

            self.assertEqual(quantize_alg, 'bitgroom')
            self.assertTrue('libnetcdf version ' in quantize_impl)


if __name__ == '__main__':
    run()
