#!/usr/bin/env python2.7
# If this example is not executed from the directory containing the
# CMOR code, please first complete the following steps:
#
#   1. In any directory, create 'Tables/', 'Test/' and 'CMIP6/'
#      directories.
#
#   2. Download CMIP6_6hrLev.json, CMIP6_CV.json,
#      CMIP6_coordinate.json and CMIP6_formula_terms.json from
#      https://github.com/PCMDI/cmip6-cmor-tables/tree/master/Tables to
#      the 'Tables/' directory.
#
#   3. Download common_user_input.json from
#      https://github.com/PCMDI/cmor/blob/master/Test/ to the 'Test/'
#      directory.
import numpy
import unittest
import cmor
import base_CMIP6_CV


class TestCase(base_CMIP6_CV.BaseCVsTest):
    def testCMIP6(self):
        try:
            inpath = 'Tables'  # 01.00.27b1
            cmor.setup(inpath=inpath, netcdf_file_action=cmor.CMOR_REPLACE,
                    logfile=self.tmpfile)
            error_flag = cmor.dataset_json('Test/common_user_input.json')
            table_id = cmor.load_table('CMIP6_6hrLev_bad_specs.json')
            time = cmor.axis(table_entry='time1', units='days since 2000-01-01',
                            coord_vals=numpy.array(range(1)),
                            cell_bounds=numpy.array(range(2)))
            latitude = cmor.axis(table_entry='latitude', units='degrees_north',
                                coord_vals=numpy.array(range(5)),
                                cell_bounds=numpy.array(range(6)))
            longitude = cmor.axis(table_entry='longitude', units='degrees_east',
                                coord_vals=numpy.array(range(5)),
                                cell_bounds=numpy.array(range(6)))
            plev3 = cmor.axis(table_entry='plev3', units='Pa',
                            coord_vals=numpy.array([85000., 50000., 25000.]))
            axis_ids = [longitude, latitude, plev3, time]
            ua_var_id = cmor.variable(table_entry='ua', axis_ids=axis_ids,
                                    units='m s-1')
            ta_var_id = cmor.variable(table_entry='ta', axis_ids=axis_ids,
                                    units='K')
            data = numpy.random.random(75)
            reshaped_data = data.reshape((5, 5, 3, 1))
            
            # This doesn't:
            cmor.write(ta_var_id, reshaped_data)
            cmor.write(ua_var_id, reshaped_data)

            #cmor.close()
        except BaseException:
            pass

        self.assertCV("data_specs_version")

if __name__ == '__main__':
    unittest.main()