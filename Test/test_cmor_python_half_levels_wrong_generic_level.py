import cdms2
import cmor
import numpy
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def read_coords(self, nlats, nlons):
        alons = (360.*numpy.arange(nlons)/nlons+.5) - 180.
        blons = numpy.zeros((nlons, 2), dtype=float)
        blons[:, 0] = 360.*numpy.arange(nlons)/nlons - 180.
        blons[:, 1] = 360.*numpy.arange(1, nlons+1)/nlons - 180.

        alats = (180.*numpy.arange(nlats)/nlats+.5) - 90.
        blats = numpy.zeros((nlats, 2), dtype=float)
        blats[:, 0] = 180.*numpy.arange(nlats)/nlats - 90.
        blats[:, 1] = 180.*numpy.arange(1, nlats+1)/nlats - 90.

        levs = numpy.array([1., 5., 10, 20, 30, 50, 70, 100, 150,
                            200, 250, 300, 400, 500, 600, 700, 850, 925, 1000])

        return alats, blats, alons, blons, levs


    def testHalfLevelsWrongGenericLevel(self):
        try:
            # PARAMETERS
            ntimes = 2
            lon = 4
            lat = 3
            plev = 19
            lev = 5

            p0 = 1.e3
            a_coeff = [0.1, 0.2, 0.3, 0.22, 0.1]
            b_coeff = [0.0, 0.1, 0.2, 0.5, 0.8]

            a_coeff_bnds = [0., .15, .25, .25, .16, 0.]
            b_coeff_bnds = [0., .05, .15, .35, .65, 1.]


            varin3d = ["MC",]

            n3d = len(varin3d)

            alats, bnds_lat, alons, bnds_lon, plevs = self.read_coords(lat, lon)

            print alats[:2], alats[-2:]
            print bnds_lat[0], bnds_lat[-1]
            print alons[:2], alons[-2:]
            print bnds_lon[0], bnds_lon[-1]
            print plevs

            ierr = cmor.setup(inpath=self.testdir,
                        netcdf_file_action=cmor.CMOR_REPLACE,
                        logfile=self.logfile)
            print("ERR:", ierr)
            ierr = cmor.dataset_json(os.path.join(self.testdir, "CMOR_input_example.json"))
            print("ERR:", ierr)

            ierr = cmor.load_table(os.path.join(self.tabledir, 'CMIP6_Amon.json'))
            print("ERR:", ierr)
            ilat = cmor.axis(
                table_entry='latitude',
                units='degrees_north',
                length=lat,
                coord_vals=alats,
                cell_bounds=bnds_lat)
            print("ILAT:", ilat)
            print(lon, alons, bnds_lon)
            ilon = cmor.axis(
                table_entry='longitude',
                coord_vals=alons,
                units='degrees_east',
                cell_bounds=bnds_lon)

            print("ILON:", ilon)

            cmor.load_table(os.path.join(self.tabledir, 'CMIP6_Omon.json'))
            itim = cmor.axis(
                table_entry="time",
                units="days since 1850",
                length=ntimes)

            print("ITIME:",itim)
            zlevs = numpy.array([.1, .3, .55, .7, .9])
            zlev_bnds = numpy.array([0., .2, .42, .62, .8, 1.])

            ilev_half = cmor.axis(table_entry='standard_hybrid_sigma',
                            units='1',
                            coord_vals=zlevs, cell_bounds=zlev_bnds)
            print("ILEVL half:",ilev_half)

            cmor.zfactor(zaxis_id=ilev_half, zfactor_name='p0', units='hPa', zfactor_values=p0)
            print "p0 1/2"
            self.processLog()
        except BaseException:
            raise

        failed = False
        try:
            cmor.zfactor(zaxis_id=ilev_half, zfactor_name='b_half', axis_ids=[ilev_half, ],
                        zfactor_values=b_coeff)
        except:
            failed = True

        if not failed:
            raise RuntimeError("This code should have error exited")

        try:
            cmor.close()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
