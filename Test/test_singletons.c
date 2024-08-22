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


int test_bs550aer(const int *axes_ids, int num_axes, int zfactor_id, double basetime)
{
    double time, time_bnds[2];
    int var_id;
    float values[8], miss = 1.e20f;
    char positive = '\0';
    int i, singleton_id;
    int scatangle_found = 0;
    int wavelength_found = 0;

    if (cmor_variable(&var_id, "bs550aer", "m-1 sr-1",
                      num_axes, (int *)axes_ids, 'f', &miss,
                      NULL, &positive, NULL, NULL, NULL) != 0)
        fail("cmor_variable(bs550aer)");

    // Find singleton dimension for bs550aer
    for(i = 0; i < num_axes; ++i){
        singleton_id = cmor_vars[var_id].singleton_ids[i];
        if(singleton_id != -1) {
            if(strcmp(cmor_axes[singleton_id].id, "wavelength") == 0)
                wavelength_found++;
        }
    }
    printf("wavelength_found = %d\n", wavelength_found);

    if(wavelength_found != 1)
        fail("error in singleton dimension");

    for (i = 0; i < 4; i++) {
        time_bnds[0] = basetime + (i * 6.0) / 24.; /* 6hr */
        time_bnds[1] = basetime + ((i + 1) * 6.0) / 24.;
        time = .5 * (time_bnds[0] + time_bnds[1]);

        fillf(values, NUM_ARRAY_ELEMENTS(values), i, 0.01);
        if (cmor_write(var_id, values, 'f', NULL, 1,
                       &time, time_bnds, NULL) != 0)
            fail("cmor_write(var_id)");
        if (cmor_write(zfactor_id, values, 'f', NULL, 1, 
                       &time, time_bnds, &var_id) != 0)
            fail("cmor_write(zfactor_id)");
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
    double alev[] = {0.996149986982346, 0.982649981975555, 
                     0.958955590856714, 0.92764596935028};
    double alev_bnds[] = {1, 0.992299973964691, 0.97299998998642,
                          0.944911191727007, 0.910380746973552};
    double scalar;
    int nlon, nlat, nlev;
    int zfactor_id;

    int axes_ids[5];
    int zfactor_axis_ids[3];
    int id_lon, id_lat, id_alev, id_time, id_extra1;

    nlon = NUM_ARRAY_ELEMENTS(lon);
    nlat = NUM_ARRAY_ELEMENTS(lat);
    nlev = NUM_ARRAY_ELEMENTS(alev);

    assert(NUM_ARRAY_ELEMENTS(lon_bnds) == nlon + 1);
    assert(NUM_ARRAY_ELEMENTS(lat_bnds) == nlat + 1);
    assert(NUM_ARRAY_ELEMENTS(alev_bnds) == nlev + 1);

    printf("Start...\n");

    /*
     * Setup time, lat, lon, and alevel.
     */
    if (cmor_axis(&id_time, "time1", "days since 1970-01-01", UNLIMITED,
                  NULL, 'd', NULL, 0, NULL) != 0)
        fail("cmor_axis(time)");

    if (cmor_axis(&id_lat, "latitude", "degrees_north", nlat,
                  lat, 'd', lat_bnds, 1, NULL) != 0)
        fail("cmor_axis(lat)");

    if (cmor_axis(&id_lon, "longitude", "degrees_east", nlon,
                  lon, 'd', lon_bnds, 1, NULL) != 0)
        fail("cmor_axis(lon)");

    if (cmor_axis(&id_alev, "standard_hybrid_sigma", "", nlev,
                  alev, 'd', alev_bnds, 1, NULL) != 0)
        fail("cmor_axis(alev)");

    /*
     * Setup zfactor.
     */
    double p0[1] = {101325.0};
    double a_val[4] = {0, 0, 36.0317993164062, 171.845031738281};
    double a_bnds[5] = {  0, 0, 0,  72.0635986328125, 271.62646484375};
    double b_val[4] = {0, 0, 36.0317993164062, 171.845031738281};
    double b_bnds[5] = {  0, 0,  0, 72.0635986328125, 271.62646484375};

    int lev_id_array[2];
    lev_id_array[0] = id_alev;
    zfactor_axis_ids[0] = id_lat;
    zfactor_axis_ids[1] = id_lon;
    zfactor_axis_ids[2] = id_time;

    if (cmor_zfactor(&zfactor_id, id_alev, (char *) "p0", (char *) "Pa", 0, 0, 
                'd', (void *) p0, NULL) != 0)
        fail("cmor_zfactor(p0)");
    if (cmor_zfactor(&zfactor_id, id_alev, (char *) "b", (char *) "", 1, &lev_id_array[0], 
                'd', (void *) b_val, (void *) b_bnds) != 0)
        fail("cmor_zfactor(b)");
    if (cmor_zfactor(&zfactor_id, id_alev, (char *) "a", (char *) "", 1, &lev_id_array[0], 
                'd', (void *) a_val, (void *) a_bnds) != 0)
        fail("cmor_zfactor(a)");
    if (cmor_zfactor(&zfactor_id, id_alev, (char *) "ps1", (char *) "Pa", 3, zfactor_axis_ids, 
                'd', NULL, NULL) != 0)
        fail("cmor_zfactor(ps1)");

    /*
     * No singleton dimensions are passed.
     * CMOR adds them automatically (in cmor_variable()).
     */
    axes_ids[0] = id_time;
    axes_ids[1] = id_lat;
    axes_ids[2] = id_lon;
    axes_ids[3] = id_alev;

    printf("test_bs550aer (1 of 3)\n");
    test_bs550aer(axes_ids, 4, zfactor_id, 360. * 10);

    /*
     * Setup singleton dimension.
     */
    scalar = 550.;
    if (cmor_axis(&id_extra1, "lambda550nm", "nm", 1,
                  &scalar, 'd', NULL, 0, NULL) != 0)
        fail("cmor_axis(lambda550nm)");

    /*
     * All axis IDs are passed.
     */
    axes_ids[0] = id_extra1;
    axes_ids[1] = id_time;
    axes_ids[2] = id_alev;
    axes_ids[3] = id_lat;
    axes_ids[4] = id_lon;

    printf("test_bs550aer (2 of 3)\n");
    test_bs550aer(axes_ids, 5, zfactor_id, 360. * 11);

    /*
     * Change the order of axis IDs in axes_ids.
     */
    axes_ids[0] = id_time;
    axes_ids[1] = id_extra1;
    axes_ids[2] = id_lat;
    axes_ids[3] = id_alev;
    axes_ids[4] = id_lon;

    printf("test_bs550aer (3 of 3)\n");
    test_bs550aer(axes_ids, 5, zfactor_id, 360. * 12);
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
