import cmor
import numpy
import unittest

# ==============================
#  main thread
# ==============================


def run():
    unittest.main()


ntimes = 1
nlat = 45
nlon = 90


class TestCase(unittest.TestCase):

    def setUp(self, *args, **kwargs):
        error_flag = cmor.setup(
            inpath='Tables',
            netcdf_file_action=cmor.CMOR_REPLACE)

        error_flag = cmor.dataset_json("Test/CMOR_input_example.json")

    def prep_var(self, var, units):
        # creates 1 degree grid
        dlat = 180 / nlat
        dlon = 360. / nlon
        alats = numpy.arange(-90 + dlat / 2., 90, dlat)
        bnds_lat = numpy.arange(-90, 90 + dlat, dlat)
        alons = numpy.arange(0 + dlon / 2., 360., dlon) - 180.
        bnds_lon = numpy.arange(0, 360. + dlon, dlon) - 180.
        cmor.load_table("Tables/CMIP6_Omon.json")
        # cmor.load_table("Test/IPCC_table_A1")
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

        itim = cmor.axis(
            table_entry='time',
            units='days since 2010-1-1')

        ivar1 = cmor.variable(
            var,
            axis_ids=[itim, ilat, ilon],
            units=units,
            missing_value=0.)

        return ivar1

    def testCMIP6(self):
        try:

            for d in range(2):
                mode = cmor.CMOR_APPEND
                if d == 0:
                    mode = cmor.CMOR_REPLACE
                ivar = self.prep_var("sos", "practical_salinity_scale_78")
                ivar2 = self.prep_var("sos", "psu")
                ivar3 = self.prep_var("sos", "pss-78")
                for i in range(4):
                    tval = [i / 4. + d]
                    tbnd = [i / 4. + d - 0.125, i / 4. + d + 0.125]
                    print('tvar', tval)
                    print('tbnd', tbnd)
                    print('writing time:', i, i / 4.)
                    data = numpy.random.random(
                        (ntimes, nlat, nlon)) * 30. + 273
                    data = data.astype("f")
                    cmor.write(ivar, data, time_vals=tval, time_bnds=tbnd)
                    tval = [i / 4. + d + 100]
                    tbnd = [i / 4. + d + 100 - 0.125, i / 4. + d + 100 + 0.125]
                    cmor.write(ivar2, data, time_vals=tval, time_bnds=tbnd)
                    tval = [i / 4. + d + 200]
                    tbnd = [i / 4. + d + 200 - 0.125, i / 4. + d + 200 + 0.125]
                    cmor.write(ivar3, data, time_vals=tval, time_bnds=tbnd)
                    print('wrote var 1 time:', i)
                file = cmor.close(ivar, True)
                file1 = cmor.close(ivar2, True)
                file2 = cmor.close(ivar3, True)
                print('File:', file)
                print('File:', file1)
                print('File:', file2)
        except BaseException:
            raise

    def tearDown(self):
        import shutil
        shutil.rmtree("./CMIP6")


if __name__ == '__main__':
    run()
