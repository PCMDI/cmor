import cmor
import numpy
import unittest


class TestCase(unittest.TestCase):

    def testJoerg5(self):
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)

            cmor.dataset_json("Test/common_user_input.json")

            # creates 1 degree grid
            nlat = 18
            nlon = 36
            alats = numpy.arange(180) - 89.5
            bnds_lat = numpy.arange(181) - 90
            alons = numpy.arange(360) + .5
            bnds_lon = numpy.arange(361)
            cmor.load_table("Tables/CMIP6_Amon.json")
            cmor.close()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
