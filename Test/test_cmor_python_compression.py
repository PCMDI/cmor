import sys
import os
import unittest
import base_test_cmor_python
import cdms2
import cmor
import numpy


class TestCase(base_test_cmor_python.BaseCmorTest):

    def testCompression(self):
        try:
            f = cdms2.open(os.path.join(self.curdir, 'data', 'clt.nc'))

            cmor.setup(inpath=self.testdir,
                    set_verbosity=cmor.CMOR_NORMAL,
                    netcdf_file_action=cmor.CMOR_REPLACE, 
                    logfile=self.logfile)

            cmor.dataset_json(os.path.join(self.testdir, "CMOR_input_example.json"))

            cmor.load_table(os.path.join(self.tabledir, "CMIP6_Amon.json"))

            s = f("clt", slice(14))
            Saxes = s.getAxisList()

            axes = []
            for ax in Saxes[1:]:
                tmp = cmor.axis(
                    ax.id,
                    coord_vals=ax[:],
                    cell_bounds=ax.getBounds(),
                    units=ax.units)
                axes.append(tmp)

            # Now creates a dummy HUGE axis for resizing s as really big
            factor = 100
            nt = s.shape[0] * factor
            print 'nt is:', nt
            t = numpy.arange(nt)

            tmp = cmor.axis(
                'time',
                coord_vals=t,
                units=Saxes[0].units,
                cell_bounds=numpy.arange(
                    nt + 1))
            axes.insert(0, tmp)
            print axes
            var_id1 = cmor.variable(s.id, s.units, axes)
            # the one with 2 at the end is compressed
            var_id2 = cmor.variable(s.id, s.units, axes)
            sh = list(s.shape)
            sh[0] = nt
            s = numpy.resize(s, sh)
            # s=numpy.where(numpy.greater(s,100.),100,s)
            s = numpy.random.random(s.shape) * 10000.
            print s.shape
            cmor.write(var_id1, s)
            cmor.close(var_id1)
            cmor.write(var_id2, s)

            cmor.close()
            self.processLog()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
