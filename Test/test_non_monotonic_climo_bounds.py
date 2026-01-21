import cmor

# Hypothetical data are going from April 2049 thru March 2050
ntimes = 12
times = [18015,
        18045,
        18075,
        18105,
        18135,
        18165,
        18195,
        18225,
        18255,
        18285,
        18315,
        18345]
# first full djf one year later than first full mam
times_bnds = [(0, 36030),
            (30, 36060),
            (60, 36090),
            (90, 36120),
            (120, 36150),
            (150, 36180),
            (180, 36210),
            (210, 36240),
            (240, 36270),
            (270, 36300),
            (300, 36330),
            (330, 36360)]

def path_test():
    cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)

    cmor.dataset_json("Test/CMOR_input_example.json")


    table = 'CMIP6_Amon.json'
    cmor.load_table(table)
    axes = [{'table_entry': 'time2',
             'units': 'days since 2000-01-01 00:00:00',
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
             'coord_vals': [90, 88],
             'cell_bounds': [ (91, 89), (89, 87)]},
            ]

    axis_ids = list()
    for axis in axes:
        axis_id = cmor.axis(**axis)
        axis_ids.append(axis_id)
    varid = cmor.variable('co2Clim', '1.e-6', axis_ids)
    import numpy
    data = numpy.arange(ntimes*19*2)
    data.resize((ntimes, 19, 1, 2))
#    cmor.write(varid, data)
#    for i in range(len(data)):
#        cmor.write(varid, data[i], time_vals=times[i], time_bnds=times_bnds[i])
    cmor.write(varid, data, time_vals=times, time_bnds=times_bnds)
    path = cmor.close(varid, file_name=True)

    print(path)


if __name__ == '__main__':
    path_test()
