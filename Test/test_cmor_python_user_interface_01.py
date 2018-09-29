import cmor._cmor
import os
import unittest
import base_test_cmor_python

class TestCase(base_test_cmor_python.BaseCmorTest):

    def testUserInterface(self):
        try:
            from test_python_common import *  # common subroutines
            
            cmor.setup(
                inpath=self.tabledir,
                set_verbosity=cmor.CMOR_NORMAL,
                netcdf_file_action=cmor.CMOR_REPLACE,
                exit_control=cmor.CMOR_EXIT_ON_MAJOR, 
                logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            myaxes = numpy.zeros(9, dtype='i')
            myaxes2 = numpy.zeros(9, dtype='i')
            myvars = numpy.zeros(9, dtype='i')

            tables = []
            a = cmor.load_table("CMIP6_grids.json")
            tables.append(a)
            tables.append(cmor.load_table("CMIP6_Amon.json"))
            print 'Tables ids:', tables

            cmor.set_table(tables[0])

            x, y, lon_coords, lat_coords, lon_vertices, lat_vertices = gen_irreg_grid(
                lon, lat)


            myaxes[0] = cmor.axis(table_entry='y',
                                units='m',
                                coord_vals=y)
            myaxes[1] = cmor.axis(table_entry='x',
                                units='m',
                                coord_vals=x)

            grid_id = cmor.grid(axis_ids=myaxes[:2],
                                latitude=lat_coords,
                                longitude=lon_coords,
                                latitude_vertices=lat_vertices,
                                longitude_vertices=lon_vertices)
            print 'got grid_id:', grid_id
            myaxes[2] = grid_id

            mapnm = 'lambert_conformal_conic'
            params = ["standard_parallel1",
                    "longitude_of_central_meridian", "latitude_of_projection_origin",
                    "false_easting", "false_northing", "standard_parallel2"]
            punits = ["", "", "", "", "", ""]
            pvalues = [-20., 175., 13., 8., 0., 20.]
            cmor.set_grid_mapping(grid_id=myaxes[2],
                                mapping_name=mapnm,
                                parameter_names=params,
                                parameter_values=pvalues,
                                parameter_units=punits)

            cmor.set_table(tables[1])
            myaxes[3] = cmor.axis(table_entry='time',
                                units='months since 1980')

            pass_axes = [myaxes[3], myaxes[2]]
            myvars[0] = cmor.variable(table_entry='hfls',
                                    units='W m-2',
                                    axis_ids=pass_axes,
                                    positive='down',
                                    original_name='HFLS',
                                    history='no history',
                                    comment='no future'
                                    )
            for i in range(ntimes):
                data2d = read_2d_input_files(i, varin2d[0], lat, lon)
                print 'writing time: ', i, Time[i], data2d.shape, data2d
                print Time[i], bnds_time[2 * i:2 * i + 2]
                cmor.write(myvars[0], data2d, 1, time_vals=Time[i],
                        time_bnds=bnds_time[2 * i:2 * i + 2])

            cmor.close()
            self.processLog()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
