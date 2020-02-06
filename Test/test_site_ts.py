import cmor
import numpy


def cmor_initialisation():
    cmor.setup(inpath='Tables',
               netcdf_file_action=cmor.CMOR_REPLACE_3,
               create_subdirectories=0)
    cmor.dataset_json("Test/CMOR_input_example.json")


def setup_data():
    coord_vals = numpy.array(range(2), dtype=numpy.float64) + 0.5
    axes = [{'table_entry': 'time1',
             'units': 'days since 2000-01-01 00:00:00',
             },
            {'table_entry': 'site',
             'units': '',
             'coord_vals': [0]},
            {'table_entry': 'hybrid_height',
             'units': 'm',
             'coord_vals': coord_vals,
             'cell_bounds': [[x - 0.5, x + 0.5] for x in coord_vals],
             },
            ]

    values = numpy.array([0.5, 0.5], numpy.float32)
    return values, axes


def cmor_define_and_write(values, axes):
    table = 'CMIP6_CFsubhr.json'
    cmor.load_table(table)

    axis_ids = list()
    for axis in axes:
        axis_id = cmor.axis(**axis)
        axis_ids.append(axis_id)

    igrid = cmor.grid([axis_ids[1]], [0.], [0.])
    cmor.zfactor(axis_ids[2], 'b', axis_ids=[axis_ids[2]],
                 zfactor_values=numpy.array(range(2), dtype=numpy.float64), 
                 zfactor_bounds=[[x - 0.5, x + 0.5] for x in range(2)])

    cmor.zfactor(axis_ids[2], 'orog', 'm', axis_ids=[igrid],
                 zfactor_values=[0.] )

    ids_for_var = [axis_ids[0], igrid, axis_ids[2]]
    varid = cmor.variable('tnhus',
                          's-1',
                          ids_for_var,
                          history='variable history',
                          missing_value=-99,
                          )

    for time in [x * 1800. / 86400 for x in range(48)]:
        cmor.write(varid, values, time_vals=[time])
    return varid


def main():

    cmor_initialisation()
    values, axes = setup_data()
    varid = cmor_define_and_write(values, axes)
    fname = cmor.close(varid, file_name=True)
    cmor.close()


if __name__ == '__main__':

    main()
