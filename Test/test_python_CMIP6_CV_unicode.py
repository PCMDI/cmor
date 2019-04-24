# If this example is not executed from the directory containing the
# CMOR code, please first complete the following steps:
#
#   1. In any directory, create 'Tables/', 'Test/' and 'CMIP6/' directories.
#
#   2. Download
#      https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_Omon.json
#      and https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_CV.json
#      to the 'Tables/' directory.
#
#   3. Download
#      https://github.com/PCMDI/cmor/blob/master/Test/<filename>.json
#      to the 'Test/' directory.

import cmor
import numpy
import unittest
import os
import sys
import tempfile
import cdms2
import base_CMIP6_CV


# ==============================
#  main thread
# ==============================
def run():
    unittest.main()


class TestCase(base_CMIP6_CV.BaseCVsTest):
    def testCMIP6(self):
        # -------------------------------------------
        # Run CMOR using unicode strings
        # -------------------------------------------
        try:
            cmor.setup(inpath=u'Tables', netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.tmpfile)
            cmor.dataset_json(u"Test/CMOR_input_example.json")
            cmor.load_table(u"CMIP6_CFsubhr.json")

            axes = [{u'table_entry': u'time1',
                    u'units': u'days since 2000-01-01 00:00:00',
                    },
                    {u'table_entry': u'site',
                    u'units': u'',
                    u'coord_vals': [0]},
                    {u'table_entry': u'hybrid_height',
                    u'units': u'm',
                    u'coord_vals': numpy.array(range(2), dtype=numpy.float64),
                    u'cell_bounds': [[x, x + 1] for x in range(2)],
                    },
                    ]

            values = numpy.array([0.5, 0.5], numpy.float32)

            axis_ids = list()
            for axis in axes:
                axis_id = cmor.axis(**axis)
                axis_ids.append(axis_id)

            igrid = cmor.grid([axis_ids[1]], [0.], [0.])
            cmor.zfactor(axis_ids[2], u'b', axis_ids=[axis_ids[2]],
                        zfactor_values=numpy.array(range(2), dtype=numpy.float64), 
                        zfactor_bounds=[[x, x + 1] for x in range(2)])

            cmor.zfactor(axis_ids[2], u'orog', u'm', axis_ids=[igrid],
                        zfactor_values=[0.] )

            ids_for_var = [axis_ids[0], igrid, axis_ids[2]]
            varid = cmor.variable(u'tnhus',
                                u's-1',
                                ids_for_var,
                                history=u'variable history',
                                missing_value=-99,
                                )

            for time in [x * 1800. / 86400 for x in range(48)]:
                cmor.write(varid, values, time_vals=[time])
            self.delete_files += [cmor.close(varid, True)]

            cmor.close()
        except BaseException:
            raise


if __name__ == '__main__':
    run()
