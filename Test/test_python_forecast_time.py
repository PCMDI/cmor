
import cmor._cmor
import os
import unittest
import numpy
import cdms2


class TestForecastTime(unittest.TestCase):

    def setup(self):
        self.path = None

    def test_ReferenceTime(self):
        from test_python_common import Time, bnds_time, read_2d_input_files
        from test_python_common import alats, alons, bnds_lat, bnds_lon
        from test_python_common import entry2d, units2d, positive2d
        from test_python_common import varin2d, ntimes, lat, lon
        dtmp2 = 1.e-4

        myaxes = numpy.zeros(9, dtype='i')
        myvars = numpy.zeros(9, dtype='i')

        cmor.setup(
            inpath="Tables",
            set_verbosity=cmor.CMOR_NORMAL,
            netcdf_file_action=cmor.CMOR_REPLACE,
            exit_control=cmor.CMOR_EXIT_ON_MAJOR)

        cmor.dataset_json("Test/common_user_input.json")

        cmor.load_table(os.path.join("CMIP6_Amon.json"))

        # ok we need to make the bounds 2D because the cmor module "undoes
        # this"
        bnds_time = numpy.reshape(bnds_time, (bnds_time.shape[0] / 2, 2))
        bnds_lat = numpy.reshape(bnds_lat, (bnds_lat.shape[0] / 2, 2))
        bnds_lon = numpy.reshape(bnds_lon, (bnds_lon.shape[0] / 2, 2))

        id = 'time'
        units = 'months since 1980'
        myaxes[0] = cmor.axis(
            id,
            coord_vals=Time,
            units=units,
            cell_bounds=bnds_time,
            interval="1 month")

        id = 'latitude'
        units = "degrees_north"
        myaxes[1] = cmor.axis(
            id,
            coord_vals=alats,
            units=units,
            cell_bounds=bnds_lat)

        id = "longitude"
        units = "degrees_east"
        myaxes[2] = cmor.axis(
            id,
            coord_vals=alons,
            units=units,
            cell_bounds=bnds_lon)

        id = 'reftime'
        units = 'months since 1980'
        myaxes[3] = cmor.axis(
            id,
            coord_vals=[20],
            units=units)

        myvars[0] = cmor.variable(entry2d[0],
                                  units2d[0],
                                  myaxes[:4],
                                  'd',
                                  missing_value=None,
                                  tolerance=dtmp2,
                                  positive=positive2d[0],
                                  original_name=varin2d[0],
                                  history="no history",
                                  comment="no future")

        #  /* ok here we decalre a variable for region axis testing */
        for i in range(ntimes):
            data2d = read_2d_input_files(i, varin2d[0], lat, lon)
            cmor.write(myvars[0], data2d, ntimes_passed=1)

        self.path = cmor.close(var_id=0, file_name=True)

        f = cdms2.open(self.path)
        self.assertIn('reftime', f.listdimension())
        data = f['reftime']
        self.assertEqual(data[0], 600)

    def tearDown(self):
        if self.path:
            if os.path.exists(self.path):
                os.remove(self.path)


if __name__ == '__main__':
    unittest.main()
