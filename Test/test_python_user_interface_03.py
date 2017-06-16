import numpy
import cmor
from time import localtime, strftime
today = strftime("%Y%m%d", localtime())

print 'Done importing'
try:
    import cdms2
except BaseException:
    print "This test code needs cdms2 interface for i/0"
    import sys
    sys.exit()
import os

dpth = "data"
myaxes = numpy.zeros(9, dtype='i')
myaxes2 = numpy.zeros(9, dtype='i')
myvars = numpy.zeros(9, dtype='i')


def read_input(var, order=None):
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


def prep_var(data):
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


def prep_cmor():
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
    return


for var in ['tas', ]:
    print 'Testing var:', var
    orders = ['tyx...', 'txy...', 'ytx...', 'yxt...', 'xyt...', 'xty...', ]
    for o in orders:
        print '\tordering:', o
        data, data_ordered = read_input(var, order=o)
        prep_cmor()
        print data.shape
        var_id = prep_var(data)
        df = data.filled(data.missing_value)
        cmor.write(var_id, df)
        cmor.close()
#        fn = "CMIP6/CMIP/CSIRO-BOM/NICAM/piControl/r1i1p1f1/Amon/%s/gn/v%s/%s_Amon_piControl_NICAM_r1i1p1f1_gn_197901-199605.nc" %(var,today,var)
        fn = "/software/cmor3/cmor/CMIP6/CMIP6/ISMIP6/PCMDI/PCMDI-test-1-0/piControl-withism/r11i1p1f1/Amon/%s/gr/v%s/%s_Amon_PCMDI-test-1-0_piControl-withism_r11i1p1f1_gr_197901-199605.nc" % (
            var, today, var)
        f = cdms2.open(fn)
        s = f(var)
        if not numpy.allclose(s, data_ordered):
            raise "Error reordering: %s" % o
        else:
            print 'order: %s, passed' % o
        f.close()
print 'Done'
# cmor.close()
print 'Finito'
