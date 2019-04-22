/*
 * test_singletons.c -- test singleton (scalar) dimensions.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmor.h"

#define NUM_ARRAY_ELEMENTS(arr) (sizeof arr / sizeof arr[0])


static void fillf(float *out, size_t nelems, double start, double step)
{
    size_t i;

    for (i = 0; i < nelems; i++)
        out[i] = start + i * step;
}


static void fail(const char *message)
{
    fprintf(stderr, "failed: %s\n", message);
    exit(1);
}


int test_bs550aer(const int *axes_ids, int num_axes, double basetime)
{
    double time, time_bnds[2];
    int var_id;
    float values[8], miss = 1.e20f;
    char positive = '\0';
    int i;
    int scatangle_found = 0;
    int wavelength_found = 0;

    if (cmor_variable(&var_id, "bs550aer", "m-1 sr-1",
                      num_axes, (int *)axes_ids, 'f', &miss,
                      NULL, &positive, NULL, NULL, NULL) != 0)
        fail("cmor_variable(bs550aer)");

    // Find singleton dimensions for bs550aer
    for(i = 0; i < cmor_vars[var_id].ndims; ++i){
        if(strcmp(cmor_axes[cmor_vars[var_id].singleton_ids[i]].id, "scatangle") == 0)
            scatangle_found++;
        if(strcmp(cmor_axes[cmor_vars[var_id].singleton_ids[i]].id, "wavelength") == 0)
            wavelength_found++;
    }
    if(scatangle_found != 1 || wavelength_found != 1)
        fail("error in singleton dimensions");

    for (i = 0; i < 4; i++) {
        time_bnds[0] = basetime + (i * 6.0) / 24.; /* 6hr */
        time_bnds[1] = basetime + ((i + 1) * 6.0) / 24.;
        time = .5 * (time_bnds[0] + time_bnds[1]);

        fillf(values, NUM_ARRAY_ELEMENTS(values), i, 0.01);
        if (cmor_write(var_id, values, 'f', NULL, 1,
                       &time, time_bnds, NULL) != 0)
            fail("cmor_write()");
    }
    return 0;
}


static void run_test()
{
    const int UNLIMITED = 0;
    double lon[] = { 0., 90., 180., 270. };
    double lat[] = { -45., 45. };
    double lon_bnds[] = { -45., 45, 135., 225., 315. };
    double lat_bnds[] = { -90., 0, 90. };
    double scalar;
    int nlon, nlat;

    int axes_ids[5];
    int id_lon, id_lat, id_time, id_extra1, id_extra2;

    nlon = NUM_ARRAY_ELEMENTS(lon);
    nlat = NUM_ARRAY_ELEMENTS(lat);

    assert(NUM_ARRAY_ELEMENTS(lon_bnds) == nlon + 1);
    assert(NUM_ARRAY_ELEMENTS(lat_bnds) == nlat + 1);

    printf("Start...\n");

    /*
     * Setup time, lat, and lon.
     */
    if (cmor_axis(&id_time, "time", "days since 1970-01-01", UNLIMITED,
                  NULL, 'd', NULL, 0, NULL) != 0)
        fail("cmor_axis(time)");

    if (cmor_axis(&id_lat, "latitude", "degrees_north", nlat,
                  lat, 'd', lat_bnds, 1, NULL) != 0)
        fail("cmor_axis(lat)");

    if (cmor_axis(&id_lon, "longitude", "degrees_east", nlon,
                  lon, 'd', lon_bnds, 1, NULL) != 0)
        fail("cmor_axis(lon)");

    /*
     * No singleton dimensions are passed.
     * CMOR adds them automatically (in cmor_variable()).
     */
    axes_ids[0] = id_time;
    axes_ids[1] = id_lat;
    axes_ids[2] = id_lon;

    printf("test_bs550aer (1 of 3)\n");
    test_bs550aer(axes_ids, 3, 360. * 10);

    /*
     * Setup singleton dimensions.
     */
    scalar = 550.;
    if (cmor_axis(&id_extra1, "lambda550nm", "nm", 1,
                  &scalar, 'd', NULL, 0, NULL) != 0)
        fail("cmor_axis(lambda550nm)");

    scalar = 180.;
    if (cmor_axis(&id_extra2, "scatter180", "degree", 1,
                  &scalar, 'd', NULL, 0, NULL) != 0)
        fail("cmor_axis(scatter180)");

    /*
     * All axis IDs are passed.
     */
    axes_ids[0] = id_extra2;
    axes_ids[1] = id_extra1;
    axes_ids[2] = id_time;
    axes_ids[3] = id_lat;
    axes_ids[4] = id_lon;

    printf("test_bs550aer (2 of 3)\n");
    test_bs550aer(axes_ids, 5, 360. * 11);

    /*
     * Change the order of axis IDs in axes_ids.
     */
    axes_ids[0] = id_time;
    axes_ids[1] = id_extra1;
    axes_ids[2] = id_lat;
    axes_ids[3] = id_extra2;
    axes_ids[4] = id_lon;

    printf("test_bs550aer (3 of 3)\n");
    test_bs550aer(axes_ids, 5, 360. * 12);
}


int main(int argc, char **argv)
{
    int action = CMOR_REPLACE;
    int table_id;

    /*
     * Setup CMOR.
     */
    if (cmor_setup(NULL, &action, NULL, NULL, NULL, NULL) != 0)
        fail("cmor_setup()");
    if (cmor_dataset_json("Test/CMOR_input_example.json") != 0)
        fail("cmor_dataset_json()");
    if (cmor_load_table("Tables/CMIP6_6hrLev.json", &table_id) != 0)
        fail("cmor_load_table()");

    run_test();

    cmor_close();
    printf("All Tests done.\n");

    return 0;
}
