import cmor
import numpy
import unittest

class TestCase(unittest.TestCase):

    def testJoerg2(self):
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)

            cmor.dataset_json("Test/common_user_input.json")

            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time", units='months since 2010', coord_vals=numpy.array(
                [0, 1, 2, 3, 4.]), cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
            ivar = cmor.variable(table_entry="masso", axis_ids=[itime], units='kg')

            data = numpy.random.random(5)
            for i in range(0, 5):
                # ,time_vals=numpy.array([i,]),time_bnds=numpy.array([i,i+1]))
                cmor.write(ivar, data[i:i])
            cmor.close()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
