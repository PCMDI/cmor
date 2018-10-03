import cmor
import numpy
import sys
import os
import unittest
import base_test_cmor_python
from time import localtime, strftime
import cdms2


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testChunking(self):
        try:
            cdms2.setNetcdfShuffleFlag(0)
            cdms2.setNetcdfDeflateFlag(0)
            cdms2.setNetcdfDeflateLevelFlag(0)

            cmor.setup(
                inpath=self.tabledir,
                set_verbosity=cmor.CMOR_NORMAL,
                netcdf_file_action=cmor.CMOR_REPLACE_4,
                exit_control=cmor.CMOR_EXIT_ON_MAJOR,
                logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            tables = []
            tables.append(cmor.load_table("CMIP6_chunking.json"))
            print 'Tables ids:', tables


            # read in data, just one slice
            f = cdms2.open(os.path.join(self.curdir, 'data', 'tas_ccsr-95a.xml'))
            s = f("tas", time=slice(0, 12), squeeze=1)
            ntimes = 12
            varout = 'tas'

            myaxes = numpy.arange(10)
            myvars = numpy.arange(10)
            myaxes[0] = cmor.axis(table_entry='latitude',
                                units='degrees_north',
                                coord_vals=s.getLatitude()[:], cell_bounds=s.getLatitude().getBounds())
            myaxes[1] = cmor.axis(table_entry='longitude',
                                units='degrees_north',
                                coord_vals=s.getLongitude()[:], cell_bounds=s.getLongitude().getBounds())


            myaxes[2] = cmor.axis(table_entry='time',
                                units=s.getTime().units,
                                coord_vals=s.getTime()[:], cell_bounds=s.getTime().getBounds())

            pass_axes = [myaxes[2], myaxes[0], myaxes[1]]

            myvars[0] = cmor.variable(table_entry=varout,
                                    units=s.units,
                                    axis_ids=pass_axes,
                                    original_name=s.id,
                                    history='no history',
                                    comment='testing speed'
                                    )


            cmor.write(myvars[0], s.filled(), ntimes_passed=ntimes)
            cmor.close()
            self.processLog()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
