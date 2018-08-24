import numpy
import cdms2
import cmor
import unittest
from time import localtime, strftime
import os


class TestCase(unittest.TestCase):

    def read_input(self, var, order=None):
        cmor.setup(
            inpath="Tables",
            set_verbosity=cmor.CMOR_QUIET,
            netcdf_file_action=cmor.CMOR_REPLACE,
            exit_control=cmor.CMOR_EXIT_ON_MAJOR)
        cmor.dataset_json("Test/common_user_input.json")

        tables = []
        a = cmor.load_table("CMIP6_Omon.json")
        tables.append(a)
        tables.append(cmor.load_table("CMIP6_Amon.json"))

        dpth = "data"

        f = cdms2.open(os.path.join(dpth, "%s_sample.nc" % var))
        ok = f(var)
        if order is None:
            s = f(var)
        else:
            s = f(var, order=order)
        s.units = f[var].units
        s.id = var
        f.close()
        return s, ok

    def prep_var(self, data):
        rk = data.rank()
        axes = []
        for i in range(rk):
            ax = data.getAxis(i)
            if ax.isLongitude():
                id = cmor.axis(
                    table_entry='longitude',
                    units=ax.units,
                    coord_vals=ax[:],
                    cell_bounds=ax.getBounds())
            elif ax.isLatitude():
                id = cmor.axis(
                    table_entry='latitude',
                    units=ax.units,
                    coord_vals=ax[:],
                    cell_bounds=ax.getBounds())
            else:
                id = cmor.axis(
                    table_entry=str(
                        ax.id),
                    units=ax.units,
                    coord_vals=ax[:],
                    cell_bounds=ax.getBounds())
                print i, 'units:', ax.units, ax[0]
            axes.append(id)
        var = cmor.variable(table_entry=data.id,
                            units=data.units,
                            axis_ids=numpy.array(axes),
                            missing_value=data.missing_value,
                            history="rewrote by cmor via python script")
        return var

    def testUserInterface(self):
        try:
            for var in ['tas', ]:
                print 'Testing var:', var
                orders = ['tyx...', 'txy...', 'ytx...', 'yxt...', 'xyt...', 'xty...', ]
                for o in orders:
                    print '\tordering:', o
                    data, data_ordered = self.read_input(var, order=o)
                    print data.shape
                    var_id = self.prep_var(data)
                    df = data.filled(data.missing_value)
                    cmor.write(var_id, df)
                    fn = cmor.close(var_id, True)
                    f = cdms2.open(fn)
                    s = f(var)
                    self.assertTrue(numpy.allclose(s, data_ordered), "Error reordering: %s" % o)
                    f.close()
            cmor.close()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()