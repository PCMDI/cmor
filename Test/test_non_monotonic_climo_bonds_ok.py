import cmor

# Hypothetical data are going from march 2000 thru feb 2010
times = [72, 75, 78]
# first full djf one year later than first full mam
times_bnds = [[11, 134], [2, 125], [5, 128]]


def path_test():
    cmor.setup(inpath='TestTables', netcdf_file_action=cmor.CMOR_REPLACE)

    cmor.dataset_json("Test/CMOR_input_example.json")


    table = 'CMIP6_Amon.json'
    cmor.load_table(table)
    axes = [{'table_entry': 'time2',
             'units': 'months since 2000-01-01 00:00:00',
#             'coord_vals': times,
#             'cell_bounds': times_bnds,
             },
            {'table_entry': 'plev19',
             'units': 'Pa',
             'coord_vals': [100000., 92500., 85000., 70000., 60000., 50000.,
                            40000., 30000., 25000., 20000., 15000.,
                            10000., 7000., 5000., 3000., 2000., 1000., 500, 100]},
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
    data = numpy.array([3, 4, 5])
    data.resize((3, 19, 1, 1))
#    cmor.write(varid, data)
#    for i in range(len(data)):
#        cmor.write(varid, data[i], time_vals=times[i], time_bnds=times_bnds[i])
    cmor.write(varid, data, time_vals=[12, 15, 18], time_bnds=[[10, 1234], [12, 1125], [15, 1128]])
    path = cmor.close(varid, file_name=True)

    print(path)


if __name__ == '__main__':
    path_test()
