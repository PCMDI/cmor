import cmor
import numpy

def cmor_initialisation():
    cmor.setup(inpath='Tables',
               netcdf_file_action = cmor.CMOR_REPLACE_3,
               create_subdirectories = 0)
    cmor.dataset('pre-industrial control', 'ukmo', 'HadCM3', '360_day',
                 institute_id = 'ukmo',
                 model_id = 'HadCM3',
                 history = 'some global history',
                 forcing = 'N/A',
                 parent_experiment_id = 'N/A',
                 parent_experiment_rip = 'N/A',
                 branch_time = 0.,
                 contact = 'bob',
                 outpath = 'Test')

def setup_data():
    tvals = [ 149.833333333333, 149.854166666667, 149.875, 149.895833333333,
              149.916666666667, 149.9375, 149.958333333333, 149.979166666667]
    tbnds = list(tvals)
    tbnds.append(150)
    axes = [ {'table_entry': 'time1',
              'units': 'days since 2000-01-01 00:00:00',
              'coord_vals' : tvals,
              'cell_bounds' : tbnds,
              },
             {'table_entry': 'site',
              'units': '',
              'coord_vals': [0]},
             {'table_entry': 'smooth_level',
              'units': 'm',
              'coord_vals': [10],
              'cell_bounds': [5,15]},
             ]

    values = numpy.array([215.], numpy.float32)
    return values, axes

def cmor_define_and_write(values, axes):
    table = 'CMIP5_cfSites'
    cmor.load_table(table)
    lev_axis_id = cmor.axis(**axes[2])
    site_axis_id = cmor.axis(**axes[1])

    time_axis_id = cmor.axis(**axes[0])

    gid = cmor.grid([site_axis_id,],latitude=numpy.array([-20,]),longitude=numpy.array([150,]))


    axis_ids = [time_axis_id,gid,lev_axis_id]
    varid = cmor.variable('tnhus',
                          's-1',
                          axis_ids,
                          history = 'variable history',
                          missing_value = -99,
                          )

    cmor.write(varid, values)
    
    
def main():
    
    cmor_initialisation()
    values, axes = setup_data()
    cmor_define_and_write(values, axes)
    print cmor.close(file_name=True)
    
if __name__ == '__main__':

    main()
