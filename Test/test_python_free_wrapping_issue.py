# This is a dummy version of the ACCESS Post Processor.
# Peter Uhe 24 July 2014
# Martin Dix 21 Nov 2014
#
import numpy as np
import datetime
import cmor


def save(opts, threeD=True):

    cmor.setup(inpath=opts['table_path'],
               netcdf_file_action=cmor.CMOR_REPLACE_3,
               set_verbosity=cmor.CMOR_NORMAL,
               exit_control=cmor.CMOR_NORMAL,
               logfile=None, create_subdirectories=1)

    cmor.dataset_json("Test/CMOR_input_example.json")

    # Load the CMIP tables into memory.
    tables = []
    tables.append(cmor.load_table('CMIP6_grids.json'))
    tables.append(cmor.load_table(opts['cmip_table']))

    # Create the dimension axes

    # Monthly time axis
    min_tvals = []
    max_tvals = []
    cmor_tName = 'time'
    tvals = []
    axis_ids = []
    for year in range(1850, 1851):
        for mon in range(1, 13):
            tvals.append(datetime.date(year, mon, 15).toordinal() - 1)
    # set up time values and bounds
    for i, ordinaldate in enumerate(tvals):
        model_date = datetime.date.fromordinal(int(ordinaldate) + 1)
        # min bound is first day of month
        model_date = model_date.replace(day=1)
        min_tvals.append(model_date.toordinal() - 1)
        # max_bound is first day of next month
        tyr = model_date.year + model_date.month / 12
        tmon = model_date.month % 12 + 1
        model_date = model_date.replace(year=tyr, month=tmon)
        max_tvals.append(model_date.toordinal() - 1)
        # correct date to middle of month
        mid = (max_tvals[i] - min_tvals[i]) / 2.
        tvals[i] = min_tvals[i] + mid
    tval_bounds = np.column_stack((min_tvals, max_tvals))
    cmor.set_table(tables[1])
    time_axis_id = cmor.axis(table_entry=cmor_tName,
                             units='days since 0001-01-01', length=len(tvals),
                             coord_vals=tvals[:], cell_bounds=tval_bounds[:],
                             interval=None)
    axis_ids.append(time_axis_id)

    if not threeD:
        # Pressure
        plev = np.array([100000, 92500, 85000, 70000, 60000, 50000,
                         40000, 30000, 25000, 20000, 15000, 10000,
                         7000, 5000, 3000, 2000, 1000, 500, 100])
        plev_bounds = np.array([
            [103750, 96250],
            [96250, 88750],
            [88750, 77500],
            [77500, 65000],
            [65000, 55000],
            [55000, 45000],
            [45000, 35000],
            [35000, 27500],
            [27500, 22500],
            [22500, 17500],
            [17500, 12500],
            [12500, 8500],
            [8500, 6000],
            [6000, 4000],
            [4000, 2500],
            [2500, 1500],
            [1500, 750],
            [750, 300],
            [300, 0]])
        plev_axis_id = cmor.axis(table_entry='plev19',
                                 units='Pa', length=len(plev),
                                 coord_vals=plev[:], cell_bounds=plev_bounds[:],
                                 interval=None)
        axis_ids.append(plev_axis_id)

    # 1 degree resolution latitude and longitude
    lat = np.linspace(-89.5, 89.5, 180)
    lat_bounds = np.column_stack((np.linspace(-90., 89., 180.),
                                  np.linspace(-89., 90., 180.)))
    lat_axis_id = cmor.axis(table_entry='latitude',
                            units='degrees_north', length=len(lat),
                            coord_vals=lat[:], cell_bounds=lat_bounds[:],
                            interval=None)
    axis_ids.append(lat_axis_id)

    lon = np.linspace(0.5, 359.5, 360)
    lon_bounds = np.column_stack((np.linspace(0., 359., 360.),
                                  np.linspace(1., 360., 360.)))
    lon_axis_id = cmor.axis(table_entry='longitude',
                            units='degrees_north', length=len(lon),
                            coord_vals=lon[:], cell_bounds=lon_bounds[:],
                            interval=None)
    axis_ids.append(lon_axis_id)

    #
    # Define the CMOR variable.
    #
    cmor.set_table(tables[1])
    in_missing = float(1.e20)
    if threeD:
        variable_id = cmor.variable(table_entry='ts', units='K',
                                    axis_ids=axis_ids, type='f', missing_value=in_missing)
    else:
        variable_id = cmor.variable(table_entry='ta', units='K',
                                    axis_ids=axis_ids, type='f', missing_value=in_missing)

    #
    # Write the data
    #
    if threeD:
        data_vals = np.zeros(
            (len(tvals), len(lat), len(lon)), np.float32) + 290.
    else:
        data_vals = np.zeros(
            (len(tvals),
             len(plev),
                len(lat),
                len(lon)),
            np.float32) + 290.
    try:
        print 'writing...'
        cmor.write(variable_id, data_vals[:], ntimes_passed=np.shape(
            data_vals)[0])  # assuming time is the first dimension
    except Exception as e:
        raise Exception("ERROR writing data!")

    try:
        path = cmor.close(variable_id, file_name=True)
    except BaseException:
        raise Exception("ERROR closing cmor file!")

    print path


if __name__ == "__main__":

    opts = {'cmip_table': 'CMIP6_Amon.json',
            'outpath': 'Test',
            'table_path': 'Tables'}

    save(opts, threeD=True)
    save(opts, threeD=False)
