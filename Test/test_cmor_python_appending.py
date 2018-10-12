import cmor
import numpy
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def mywrite(self, lat=90, lon=180, data=None, time_vals=None, append_to=None, cell_bounds=None):
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

        ipth = self.testdir
        if append_to is None:
            mode = cmor.CMOR_REPLACE
        else:
            mode = cmor.CMOR_APPEND
        print 'Mode in python:', mode
        cmor.setup(inpath=ipth,
                set_verbosity=cmor.CMOR_NORMAL,
                netcdf_file_action=mode,
                logfile=self.logfile)

        cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

        cmor.load_table(os.path.join(self.tabledir, "CMIP6_Amon.json"))

        dlat = 180 / lat
        dlon = 360. / lon
        lats = numpy.arange(-90 + dlat / 2., 90, dlat)
        bnds_lat = numpy.arange(-90, 90 + dlat, dlat)
        lons = numpy.arange(0 + dlon / 2., 360., dlon) - 180.
        bnds_lon = numpy.arange(0, 360. + dlon, dlon) - 180.

        plevs = numpy.array([100000., 92500., 85000., 70000.,
                            60000., 50000., 40000., 30000., 25000., 20000.,
                            15000., 10000., 7000., 5000., 3000., 2000., 1000., 500., 100.])

        itim = cmor.axis(table_entry='time', units='month since 2008')
        #itim = cmor.axis(table_entry='time',units='month since 2008',coord_vals=numpy.arange(0,12,1))
        ilat = cmor.axis(
            table_entry='latitude',
            coord_vals=lats,
            units='degrees_north',
            cell_bounds=bnds_lat)
        ilon = cmor.axis(
            table_entry='longitude',
            coord_vals=lons,
            units='degrees_east',
            cell_bounds=bnds_lon)
        print 'so far', itim, ilat, ilon
        ilev = cmor.axis(table_entry="plev19", coord_vals=plevs, units="Pa")

        iv = cmor.variable(
            table_entry='ta', axis_ids=numpy.array(
                (itim, ilev, ilat, ilon)), units='K')

        # cmor.write(iv,data)
        if append_to is None:
            print 'time:', time_vals
            print 'bnds:', cell_bounds
            # ,file_suffix="with-appending")
            cmor.write(iv, data, time_vals=time_vals, time_bnds=cell_bounds)
        else:
            print 'Ok writing with a suffix', append_to
            cmor.write(
                iv,
                data,
                time_vals=time_vals,
                file_suffix=append_to,
                time_bnds=cell_bounds)
            print 'and back'
        file = cmor.close(iv, file_name=True)
        print 'Ok dumped to:', file
        cmor.close()
        self.processLog()
        return file

    def testAppending(self):
        try:
            nlat = 90
            nlon = 180
            ntime = 12
            ndata = numpy.random.random((ntime, 19, nlat, nlon)) + 280.

            f1 = self.mywrite(lat=nlat, lon=nlon, data=ndata[:6], time_vals=numpy.arange(0, 6, 1), cell_bounds=numpy.arange(0, 7, 1))
            print 'First part: ', f1
            f2 = self.mywrite(lat=nlat, lon=nlon, data=ndata[6:], time_vals=numpy.arange(6, 12, 1), cell_bounds=numpy.arange(6, 13, 1), append_to=f1)
            print f2
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
