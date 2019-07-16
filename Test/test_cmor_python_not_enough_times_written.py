import cmor
import numpy
import unittest
import sys
import os
import tempfile
import re

class TestCase(unittest.TestCase):

    def setUp(self, *args, **kwargs):
        # create temporary log file
        self.logfile = tempfile.mkstemp()[1]

    def tearDown(self):
        os.unlink(self.logfile)

    def testNotEnoughTimesWritten(self):
        try:

            time_len = 5
            time_vals = numpy.arange(time_len)
            time_bnds = numpy.arange(time_len+1)

            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)
            cmor.dataset_json("Test/CMOR_input_example.json")
            cmor.set_cur_dataset_attribute(
                "source_type", "AOGCM ISM")

            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time", units='months since 2010',
                              coord_vals=time_vals,
                              cell_bounds=time_bnds)
            ivar = cmor.variable(
                table_entry="masso",
                axis_ids=[itime],
                units='kg')

            data_len = time_len - 1
            data = numpy.random.random(data_len)
            for i in range(0, data_len):
                a = cmor.write(ivar, data[i:i+1], ntimes_passed=1)
                
            cmor.close()

            # find "not enough times written" warning in log
            warn_msg = "! Warning: while closing variable {} (masso, table Omon)\n" \
                       "! we noticed you wrote {} time steps for the variable,\n" \
                       "! but its time axis {} (time) has {} time steps" \
                        .format(ivar, data_len, itime, time_len)
            remove_ansi = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
            with open(self.logfile) as f:
                lines = f.readlines()
                log_text = remove_ansi.sub('', ''.join(lines))
                self.assertIn(warn_msg, log_text)

        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
