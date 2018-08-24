import cmor
import numpy
import unittest


class TestCase(unittest.TestCase):

    def testJoerg7(self):
        try:
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)

            cmor.dataset_json("Test/common_user_input.json")


            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time", units='months since 2010', coord_vals=numpy.array(
                [0, 1, 2, 3, 4.]), cell_bounds=numpy.array([0, 1, 2, 3, 4, 5.]))
            # creates 1 degree grid
            nlat = 18
            nlon = 36
            alats = numpy.arange(180) - 89.5
            bnds_lat = numpy.arange(181) - 90
            alons = numpy.arange(360) + .5
            bnds_lon = numpy.arange(361)
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
            ivar = cmor.variable(
                table_entry="eparag100",
                axis_ids=[
                    itime,
                    ilat,
                    ilon],
                units='mol m-2 s-1',
                positive="up")

            data = numpy.random.random((5, nlat, nlon))
            for i in range(0, 5):
                # ,time_vals=numpy.array([i,]),time_bnds=numpy.array([i,i+1]))
                cmor.write(ivar, data[i:i])
            cmor.close()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
