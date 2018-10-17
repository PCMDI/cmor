import cmor
import numpy
import os
import unittest
import base_test_cmor_python

class TestCase(base_test_cmor_python.BaseCmorTest):

    def testNewTable(self):
        try:
            ipth = self.tabledir
            cmor.setup(inpath=ipth,
                    set_verbosity=cmor.CMOR_NORMAL,
                    netcdf_file_action=cmor.CMOR_REPLACE,
                    logfile=self.logfile)

            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            cmor.load_table("CMIP6_Amon.json")

            nlat = 90
            dlat = 180 / nlat
            nlon = 180
            dlon = 360. / nlon

            lats = numpy.arange(-90 + dlat / 2., 90, dlat)
            blats = numpy.arange(-90, 90 + dlat, dlat)
            lons = numpy.arange(0 + dlon / 2., 360., dlon)
            blons = numpy.arange(0, 360. + dlon, dlon)

            ntime = 12

            data = numpy.random.random((ntime, nlat, nlon)) + 280.

            itim = cmor.axis(
                table_entry='time', coord_vals=numpy.arange(
                    0, ntime, 1), units='month since 2008', cell_bounds=numpy.arange(
                        0, ntime + 1, 1))
            ilat = cmor.axis(
                table_entry='latitude',
                coord_vals=lats,
                units='degrees_north',
                cell_bounds=blats)
            ilon = cmor.axis(
                table_entry='longitude',
                coord_vals=lons,
                units='degrees_east',
                cell_bounds=blons)

            iv = cmor.variable(
                table_entry='tas', axis_ids=numpy.array(
                    (itim, ilat, ilon)), units='K')

            cmor.write(iv, data)
            cmor.close()
            self.processLog()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
