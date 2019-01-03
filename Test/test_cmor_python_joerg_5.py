import cmor
import numpy
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testJoerg5(self):
        try:
            cmor.setup(inpath=self.tabledir, netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)

            cmor.dataset_json(os.path.join(self.testdir, "CMOR_input_example.json"))

            # creates 1 degree grid
            nlat = 18
            nlon = 36
            alats = numpy.arange(180) - 89.5
            bnds_lat = numpy.arange(181) - 90
            alons = numpy.arange(360) + .5
            bnds_lon = numpy.arange(361)
            cmor.load_table(os.path.join(self.tabledir, "CMIP6_Amon.json"))
            cmor.close()
            self.processLog()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
