import cmor
import numpy
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testRevertedLats(self):
        try:
            nlat = 10
            dlat = 180. / nlat
            nlon = 20
            dlon = 360. / nlon
            nlev = 17
            ntimes = 1

            lats = numpy.arange(90 - dlat / 2., -90, -dlat)
            blats = numpy.arange(90, -90 - dlat, -dlat)
            lats2 = numpy.arange(-90 + dlat / 2., 90, dlat)
            blats2 = numpy.arange(-90, 90 + dlat, dlat)
            lons = numpy.arange(0 + dlon / 2., 360., dlon)
            blons = numpy.arange(0, 360. + dlon, dlon)

            cmor.setup(inpath=self.curdir, netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "CMOR_input_example.json"))
            table = os.path.join(self.tabledir,  'CMIP6_Amon.json')
            cmor.load_table(table)

            data = lats[:, numpy.newaxis] * lons[numpy.newaxis, :]

            data = (data + 29000) / 750. + 233.2


            ilat = cmor.axis(
                table_entry='latitude',
                coord_vals=lats,
                cell_bounds=blats,
                units='degrees_north')
            ilat2 = cmor.axis(
                table_entry='latitude',
                coord_vals=lats2,
                cell_bounds=blats2,
                units='degrees_north')
            ilon = cmor.axis(
                table_entry='longitude',
                coord_vals=lons,
                cell_bounds=blons,
                units='degrees_east')
            itim = cmor.axis(table_entry='time', units='months since 2010')


            ivar = cmor.variable(
                table_entry='tasmin',
                units='K',
                axis_ids=[
                    itim,
                    ilat,
                    ilon])
            cmor.write(ivar, data, ntimes_passed=1, time_vals=[0.], time_bnds=[0., 1.])
            fnm = cmor.close(ivar, file_name=True)

            print '*******************************'

            ivar2 = cmor.variable(
                table_entry='tasmin',
                units='K',
                axis_ids=[
                    itim,
                    ilat2,
                    ilon])
            cmor.write(ivar2, data, ntimes_passed=1, time_vals=[1.], time_bnds=[1., 2.])
            fnm2 = cmor.close(ivar2, file_name=True)

            cmor.close()
            self.processLog()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
