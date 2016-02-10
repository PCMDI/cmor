import cmor
import numpy

def cmor_initialisation():
    cmor.setup(inpath='Tables',
               netcdf_file_action = cmor.CMOR_REPLACE_3,
               create_subdirectories = 0)
    cmor.dataset('abrupt4xCO2', 'ukmo', 'HadCM3', '360_day',
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
    axes = [ {'table_entry': 'time',
              'units': 'days since 2000-01-01 00:00:00'
              },
             {'table_entry': 'latitude',
              'units': 'degrees',
              'coord_vals': [0],
              'cell_bounds': [-0.5, 0.5]},
             {'table_entry': 'longitude',
              'units': 'degrees',
              'coord_vals': [361., 362.],
              'cell_bounds': [360., 361., 362.]},
             ]

    values = numpy.array([215., 216.], numpy.float32)
    return values, axes

def cmor_define_and_write(values, axes):
    table = 'CMIP6_Amon_json'
    cmor.load_table(table)

    axis_ids = list()
    for axis in axes:
       axis_ids.append(cmor.axis(**axis))
                    
    table = 'CMIP6_Amon_json'
    cmor.load_table(table)

    varid = cmor.variable('rlut',
                          'W m-2',
                          axis_ids,
                          history = 'variable history',
                          missing_value = -99,
                          positive = 'up'
                          )

    cmor.write(varid, values, time_vals = [15], time_bnds = [0, 30])

def version(cmor):
   return '%s.%s.%s' % (cmor.CMOR_VERSION_MAJOR,
                        cmor.CMOR_VERSION_MINOR,
                        cmor.CMOR_VERSION_PATCH)

def main():
    assert version(cmor) == '3.0.0'
    cmor_initialisation()
    values, axes = setup_data()
    cmor_define_and_write(values, axes)
    cmor.close()
    
if __name__ == '__main__':

    main()