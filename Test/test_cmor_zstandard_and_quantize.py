import cmor
import numpy
import unittest
import os
import shutil
import tempfile
from collections import namedtuple
from netCDF4 import Dataset


ZSTDSettings = namedtuple('ZSTDSettings', ['level'])
DeflateSettings = namedtuple('DeflateSettings', ['deflate', 'level', 'shuffle'])
QuantizeSettings = namedtuple('QuantizeSettings', ['mode', 'nsd'])


def run():
    unittest.main()


class TestCase(unittest.TestCase):

    def gen_file(self, seed, out_dir, ntimes, 
                 zstd_settings: ZSTDSettings = None,
                 deflate_settings: DeflateSettings = None,
                 quantize_settings: QuantizeSettings = None):

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

        if quantize_settings is not None:
            cmor.set_quantize(var3d_ids, quantize_settings.mode, quantize_settings.nsd)

        if deflate_settings is not None:
            use_deflate = 1 if deflate_settings.deflate > 0 else 0
            use_shuffle = 1 if deflate_settings.shuffle else 0
            cmor.set_deflate(var3d_ids, use_shuffle, use_deflate,
                            deflate_level=deflate_settings.level)
        
        if zstd_settings is not None:
            cmor.set_zstandard(var3d_ids, zstd_settings.level)

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

        dst_file = 'test_data'
        if deflate_settings is not None:
            if deflate_settings.deflate:
                dst_file += f'_deflate_level_{str(deflate_settings.level)}'
        else:
            dst_file += f'_deflate_level_1'

        if zstd_settings is not None:
            dst_file += f'_zstd_level_{str(zstd_settings.level)}'
    
        if deflate_settings is not None and deflate_settings.shuffle:
            dst_file += '_shuffle'
        if quantize_settings is not None:
            dst_file += f'_qmode_{str(quantize_settings.mode)}_nsd_{str(quantize_settings.nsd)}'
        dst_file += '.nc'

        dst_path = os.path.join(out_dir, dst_file)

        shutil.copy(nc_path, dst_path)

        return dst_path


    def test_zstandard_compression(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            default_deflate = self.gen_file(seed, tmp_dir, ntimes)
            no_compression = self.gen_file(seed, tmp_dir, ntimes,
                                           deflate_settings=DeflateSettings(False, 0, False))
            zstd_shuffle = self.gen_file(seed, tmp_dir, ntimes, 
                                         zstd_settings=ZSTDSettings(3),
                                         deflate_settings=DeflateSettings(False, 0, True))

            no_comp_size = os.path.getsize(no_compression)
            zstd_shuffle_size = os.path.getsize(zstd_shuffle)
            print(f'File size without compression: {no_comp_size} bytes')
            print(f'File size with zstandard compression and shuffle: {zstd_shuffle_size} bytes')
            self.assertTrue(zstd_shuffle_size < no_comp_size)

            default_deflate_nc = Dataset(default_deflate, "r", format="NETCDF4")
            no_comp_nc = Dataset(no_compression, "r", format="NETCDF4")
            zstd_shuffle_nc = Dataset(zstd_shuffle, "r", format="NETCDF4")

            default_deflate_ta = default_deflate_nc.variables['ta'][:]
            no_comp_ta = no_comp_nc.variables['ta'][:]
            zstd_shuffle_ta = zstd_shuffle_nc.variables['ta'][:]
            self.assertIsNone(numpy.testing.assert_array_equal(default_deflate_ta, zstd_shuffle_ta))
            self.assertIsNone(numpy.testing.assert_array_equal(no_comp_ta, zstd_shuffle_ta))

            default_deflate_filters = default_deflate_nc.variables['ta'].filters()
            self.assertTrue(default_deflate_filters['zlib'])
            self.assertFalse(default_deflate_filters['zstd'])
            self.assertFalse(default_deflate_filters['shuffle'])
            self.assertEqual(default_deflate_filters['complevel'], 1)

            no_comp_filters = no_comp_nc.variables['ta'].filters()
            self.assertFalse(no_comp_filters['zlib'])
            self.assertFalse(no_comp_filters['zstd'])
            self.assertFalse(no_comp_filters['shuffle'])
            self.assertEqual(no_comp_filters['complevel'], 0)

            zstd_shuffle_filters = zstd_shuffle_nc.variables['ta'].filters()
            self.assertFalse(zstd_shuffle_filters['zlib'])
            self.assertTrue(zstd_shuffle_filters['zstd'])
            self.assertTrue(zstd_shuffle_filters['shuffle'])
            self.assertEqual(zstd_shuffle_filters['complevel'], 3)


    def test_no_zstandard_compression_if_deflate_still_enabled(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            filename = self.gen_file(seed, tmp_dir, ntimes,
                                     zstd_settings=ZSTDSettings(3))

            nc = Dataset(filename, "r", format="NETCDF4")

            filters = nc.variables['ta'].filters()
            self.assertTrue(filters['zlib'])
            self.assertFalse(filters['zstd'])
            self.assertFalse(filters['shuffle'])
            self.assertEqual(filters['complevel'], 1)


    def test_no_zstandard_compression_if_level_out_of_range(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            filename = self.gen_file(seed, tmp_dir, ntimes,
                                     zstd_settings=ZSTDSettings(100),
                                     deflate_settings=DeflateSettings(False, 0, True))

            nc = Dataset(filename, "r", format="NETCDF4")

            filters = nc.variables['ta'].filters()
            self.assertFalse(filters['zlib'])
            self.assertFalse(filters['zstd'])
            self.assertTrue(filters['shuffle'])
            self.assertEqual(filters['complevel'], 0)


    def test_quantize(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            default = self.gen_file(seed, tmp_dir, ntimes,
                                    quantize_settings=QuantizeSettings(0, 0))
            quantized = self.gen_file(seed, tmp_dir, ntimes,
                                      quantize_settings=QuantizeSettings(1, 4))

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

            quantized_attr = quantized_ta.getncattr('quantization')
            quantized_nsd = quantized_ta.getncattr('quantization_nsd')

            self.assertEqual(quantized_attr, 'quantization_info')
            self.assertEqual(quantized_nsd, 4)

            quantized_info = quantized_nc.variables['quantization_info']
            quantized_alg = quantized_info.getncattr('algorithm')
            quantized_impl = quantized_info.getncattr('implementation')

            self.assertEqual(quantized_alg, 'bitgroom')
            self.assertTrue('libnetcdf version ' in quantized_impl)


    def test_quantization_modes(self):

        seed = 123
        ntimes = 100

        with tempfile.TemporaryDirectory() as tmp_dir:
            granular = self.gen_file(seed, tmp_dir, ntimes,
                                     quantize_settings=QuantizeSettings(2, 3))
            bitround = self.gen_file(seed, tmp_dir, ntimes,
                                     quantize_settings=QuantizeSettings(3, 6))

            granular_nc = Dataset(granular, "r", format="NETCDF4")
            bitround_nc = Dataset(bitround, "r", format="NETCDF4")

            granular_ta = granular_nc.variables['ta']
            bitround_ta = bitround_nc.variables['ta']

            granular_attributes = granular_ta.ncattrs()

            self.assertTrue('quantization_info' in granular_nc.variables)
            self.assertTrue('quantization' in granular_attributes)
            self.assertTrue('quantization_nsd' in granular_attributes)
            self.assertTrue('quantization_nsb' not in granular_attributes)

            granular_attr = granular_ta.getncattr('quantization')
            granular_nsd = granular_ta.getncattr('quantization_nsd')

            self.assertEqual(granular_attr, 'quantization_info')
            self.assertEqual(granular_nsd, 3)

            granular_info = granular_nc.variables['quantization_info']
            granular_alg = granular_info.getncattr('algorithm')
            granular_impl = granular_info.getncattr('implementation')

            self.assertEqual(granular_alg, 'granular_bitround')
            self.assertTrue('libnetcdf version ' in granular_impl)

            bitround_attributes = bitround_ta.ncattrs()

            self.assertTrue('quantization_info' in bitround_nc.variables)
            self.assertTrue('quantization' in bitround_attributes)
            self.assertTrue('quantization_nsd' not in bitround_attributes)
            self.assertTrue('quantization_nsb' in bitround_attributes)

            bitround_attr = bitround_ta.getncattr('quantization')
            bitround_nsd = bitround_ta.getncattr('quantization_nsb')

            self.assertEqual(bitround_attr, 'quantization_info')
            self.assertEqual(bitround_nsd, 6)

            bitround_info = bitround_nc.variables['quantization_info']
            bitround_alg = bitround_info.getncattr('algorithm')
            bitround_impl = bitround_info.getncattr('implementation')

            self.assertEqual(bitround_alg, 'bitround')
            self.assertTrue('libnetcdf version ' in bitround_impl)


if __name__ == '__main__':
    run()
