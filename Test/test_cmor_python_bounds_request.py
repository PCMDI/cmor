import cmor
import numpy
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testBoundsRequest(self):
        try:
            breq = "100000. 80000. 80000. 68000. 68000. 56000. 56000. 44000. 44000. 31000. 31000. 18000. 18000.  0.".split()

            bnds_req = []
            for b in breq:
                bnds_req.append(float(b))

            bnds_req = numpy.array(bnds_req)
            bnds_req.shape = (7, 2)

            print bnds_req[-2], bnds_req.shape

            levs = []

            for b in bnds_req:
                levs.append((b[0] + b[1]) / 2.)

            levs = numpy.array(levs)

            print levs

            cmor.setup(inpath=self.tabledir,
                    set_verbosity=cmor.CMOR_NORMAL,
                    netcdf_file_action=cmor.CMOR_REPLACE,
                    logfile=self.logfile)

            cmor.dataset_json(os.path.join(self.testdir, "common_user_inputNOBOUNDS.json"))

            cmor.load_table(os.path.join(self.curdir, "TestTables", "python_test_table_A"))

            nlat = 90
            dlat = 180 / nlat
            nlon = 180
            dlon = 360. / nlon

            lats = numpy.arange(-90 + dlat / 2., 90, dlat)
            lons = numpy.arange(0, 360., dlon)

            ntime = 12

            data = numpy.random.random((ntime, 7, nlat, nlon)) + 280.

            itim = cmor.axis(
                table_entry='time',
                coord_vals=numpy.arange(
                    0,
                    ntime,
                    1),
                units='month since 2008')
            ilat = cmor.axis(
                table_entry='latitude',
                coord_vals=lats,
                units='degrees_north')
            ilon = cmor.axis(
                table_entry='longitude',
                coord_vals=lons,
                units='degrees_east')
            print 'so far', itim, ilat, ilon
            print bnds_req
            ilev = cmor.axis(
                table_entry="pressure2",
                coord_vals=levs,
                cell_bounds=bnds_req,
                units="Pa")

            iv = cmor.variable(
                table_entry='ta', axis_ids=numpy.array(
                    (itim, ilev, ilat, ilon)), units='K')

            cmor.write(iv, data)
            cmor.close()
            self.processLog()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
