import cmor
import numpy
import unittest

class TestAssociatedVariable(unittest.TestCase):

    def test_writing_dataset_with_associated_variable(self):
        data = 10. * numpy.random.random_sample((2, 3, 4)) + 250.
        data = numpy.array([
            72.8, 73.2, 73.6, 74,
            71.6, 72, 72.4, 72.4,
            70.4, 70.8, 70.8, 71.2,
            67.6, 69.2, 69.6, 70,
            66, 66.4, 66.8, 67.2,
            64.8, 65.2, 65.6, 66,
            63.6, 64, 64.4, 64.4,
            60.8, 61.2, 62.8, 63.2,
            59.6, 59.6, 60, 60.4,
            58, 58.4, 58.8, 59.2,
            56.8, 57.2, 57.6, 58,
            54, 54.4, 54.8, 56.4,
            52.8, 53.2, 53.2, 53.6,
            51.6, 51.6, 52, 52.4,
            50, 50.4, 50.8, 51.2,
            72.9, 73.3, 73.7, 74.1,
            71.7, 72.1, 72.5, 72.5,
            70.5, 70.9, 70.9, 71.3,
            67.7, 69.3, 69.7, 70.1,
            66.1, 66.5, 66.9, 67.3,
            64.9, 65.3, 65.7, 66.1,
            63.7, 64.1, 64.5, 64.5,
            60.9, 61.3, 62.9, 63.3,
            59.7, 59.7, 60.1, 60.5,
            58.1, 58.5, 58.9, 59.3,
            56.9, 57.3, 57.7, 58.1,
            54.1, 54.5, 54.9, 56.5,
            52.9, 53.3, 53.3, 53.7,
            51.7, 51.7, 52.1, 52.5,
            50.1, 50.5, 50.9, 51.3])
        data.shape = (2, 5, 3, 4)
        lat = numpy.array([10, 20, 30])
        lat_bnds = numpy.array([5, 15, 25, 35])
        lon = numpy.array([0, 90, 180, 270])
        lon_bnds = numpy.array([-45, 45,
                                135,
                                225,
                                315
                                ])
        time = numpy.array([15.5, 45])
        time_bnds = numpy.array([0, 31, 60])
        lev = [0.92, 0.72, 0.5, 0.3, 0.1]
        lev_bnds = [1, 0.83,
                    0.61,
                    0.4,
                    0.2,
                    0
                    ]
        p0 = 100000
        a = [0.12, 0.22, 0.3, 0.2, 0.1]
        b = [0.8, 0.5, 0.2, 0.1, 0]
        ps = numpy.array([
            97000, 97400, 97800, 98200,
            98600, 99000, 99400, 99800,
            100200, 100600, 101000, 101400,
            97100, 97500, 97900, 98300,
            98700, 99100, 99500, 99900,
            100300, 100700, 101100, 101500])
        ps.shape = (2, 3, 4)
        a_bnds = [
            0.06, 0.18,
            0.26,
            0.25,
            0.15,
            0]
        b_bnds = [
            0.94, 0.65,
            0.35,
            0.15,
            0.05,
            0]

        cmor.setup(inpath='Tables',
                set_verbosity=cmor.CMOR_NORMAL,
                netcdf_file_action=cmor.CMOR_REPLACE)
        cmor.dataset_json("Test/CMOR_input_example.json")
        cmor.load_table("CMIP6_Amon.json")
        cmorLat = cmor.axis("latitude",
                            coord_vals=lat,
                            cell_bounds=lat_bnds,
                            units="degrees_north")
        cmorLon = cmor.axis("longitude",
                            coord_vals=lon,
                            cell_bounds=lon_bnds,
                            units="degrees_east")
        cmorTime = cmor.axis("time",
                            coord_vals=time,
                            cell_bounds=time_bnds,
                            units="days since 2018")
        cmorLev = cmor.axis("standard_hybrid_sigma",
                            units='1',
                            coord_vals=lev,
                            cell_bounds=lev_bnds)
        axes = [cmorTime, cmorLev, cmorLat, cmorLon]
        _ = cmor.zfactor(zaxis_id=cmorLev,
                         zfactor_name='a',
                         axis_ids=[cmorLev, ],
                         zfactor_values=a,
                         zfactor_bounds=a_bnds)
        _ = cmor.zfactor(zaxis_id=cmorLev,
                         zfactor_name='b',
                         axis_ids=[cmorLev, ],
                         zfactor_values=b,
                         zfactor_bounds=b_bnds)
        _ = cmor.zfactor(zaxis_id=cmorLev,
                         zfactor_name='p0',
                         units='Pa',
                         zfactor_values=p0)
        ips = cmor.zfactor(zaxis_id=cmorLev,
                           zfactor_name='ps',
                           axis_ids=[cmorTime, cmorLat, cmorLon],
                           units='Pa')
        cmorVar = cmor.variable("cl", "%", axes)
        cmor.write(cmorVar, data)
        cmor.write(ips, ps, store_with=cmorVar)
        filename = cmor.close(cmorVar, file_name=True)
        cmor.close()


if __name__ == '__main__':
    unittest.main()
