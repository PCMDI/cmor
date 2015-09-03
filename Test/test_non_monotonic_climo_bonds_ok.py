import cmor

## Hypothetical data are going from march 2000 thru feb 2010
times = [72,75,78]
times_bnds = [[11,134],[2,125],[5,128]] # first full djf one year later than first full mam


def path_test():
    cmor.setup(inpath='TestTables',netcdf_file_action=cmor.CMOR_REPLACE)

    cmor.dataset('historical', 'ukmo', 'HadCM3', '360_day',model_id='HadCM3',forcing='Nat',
                 contact="J.T. Snow",
                 institute_id="PCMDI",
                 parent_experiment_id="N/A",
                 parent_experiment_rip="N/A",
                 branch_time=0)
    
    table='CMIP5_Amon'
    cmor.load_table(table)
    axes = [ {'table_entry': 'time2',
              'units': 'months since 2000-01-01 00:00:00',
              'coord_vals':  times,
              'cell_bounds': times_bnds,
              },
             {'table_entry': 'plevs',
              'units': 'Pa',
              'coord_vals': [100000., 92500., 85000., 70000., 60000., 50000., 
                40000., 30000., 25000., 20000., 15000.,
                10000., 7000., 5000., 3000., 2000., 1000.]},
             {'table_entry': 'latitude',
              'units': 'degrees_north',
              'coord_vals': [0],
              'cell_bounds': [-1, 1]},             
             {'table_entry': 'longitude',
              'units': 'degrees_east',
              'coord_vals': [90],
              'cell_bounds': [89, 91]},
             ]
              
    axis_ids = list()
    for axis in axes:
        axis_id = cmor.axis(**axis)
        axis_ids.append(axis_id)
    varid = cmor.variable('co2Clim', '1.e-6', axis_ids)
    import numpy
    data =numpy.array([3,4,5])
    data.resize((3,17,1,1))
    cmor.write(varid, data)
    path=cmor.close(varid, file_name=True)

    print path

if __name__ == '__main__':
    path_test()
