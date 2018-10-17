import cmor
import numpy
import os
import unittest
import base_test_cmor_python

class TestCase(base_test_cmor_python.BaseCmorTest):

    def testJoerg6(self):
        try:
            cmor.setup(inpath=self.tabledir, netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)

            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time", units='months since 2010', coord_vals=numpy.array(
                [0, 1, 2, 3, 4.]), cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
            ivar = cmor.variable(table_entry="masso", axis_ids=[itime], units='kg')

            data = numpy.random.random(5)
            for i in range(0, 5):
                # ,time_vals=numpy.array([i,]),time_bnds=numpy.array([i,i+1]))
                cmor.write(ivar, data[i:i])
            cmor.close()
            self.processLog()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
