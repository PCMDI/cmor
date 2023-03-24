#include <time.h>
#include <stdio.h>
#include<string.h>
#include "cmor.h"
#include <stdlib.h>
#include <math.h>

void read_time(it, time, time_bnds)
int it;
double time[];
double time_bnds[];
{
    time[0] = (it - 0.5) * 0.25;
    time_bnds[0] = (it - 1) * 0.25;
    time_bnds[1] = it * 0.25;
}

#include "reader_2D_3D.h"

void read_coords(alats, alons, plevs, bnds_lat, bnds_lon, lon, lat, lev)
double *alats, *alons;
int *plevs;
double *bnds_lat, *bnds_lon;
int lon, lat, lev;
{
    int i;

    for (i = 0; i < lon; i++) {
        alons[i] = i * 360. / lon;
        bnds_lon[2 * i] = (i - 0.5) * 360. / lon;
        bnds_lon[2 * i + 1] = (i + 0.5) * 360. / lon;
    };

    for (i = 0; i < lat; i++) {
        alats[i] = (lat - i) * 10;
        bnds_lat[2 * i] = (lat - i) * 10 + 5.;
        bnds_lat[2 * i + 1] = (lat - i) * 10 - 5.;
    };

    for (i = 0; i < lev; i++) {
        plevs[i] = (i + 1) * 100;
    }
}

int main()
{
    /*   dimension parameters: */
    /* --------------------------------- */
#define ntimes  2            /* number of time samples to process */
#define lon  3                /* number of longitude grid cells   */
#define lat  4                /* number of latitude grid cells */
#define lev  4                /* number of standard pressure levels */

    double iplevs[lev];
    double lon_coords[lon];
    double lat_coords[lat];
    double lon_bounds[lon * 2];
    double lat_bounds[lat * 2];

    double data2d[lat * lon];
    double data3d[lev * lat * lon];

    int myvars[10];
    int zfactor_id;
    int tables[4];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    int i, j, k;
    int ierr = 0;

    double Time[ntimes];
    double bnds_time[ntimes * 2];
    double tolerance = 1.e-4;
    double lon0 = 280.;
    double lat0 = 0.;
    double delta_lon = 10.;
    double delta_lat = 10.;
    char id[CMOR_MAX_STRING];
    double tmpf = 0.;
    char returnvalue[CMOR_MAX_STRING];

    printf("Testing variable with both singleton and generic dimensions.\n");

    int exit_mode = CMOR_EXIT_ON_MAJOR;
    int cmor_mode = CMOR_REPLACE;
    ierr |= cmor_setup(NULL, &cmor_mode, NULL, &exit_mode, NULL, NULL);
    ierr |= cmor_dataset_json("Test/CMOR_input_example.json");
    ierr |= cmor_load_table("Tables/CMIP6_6hrLev.json", &tables[1]);

    for (i = 0; i < ntimes; i++)
        read_time(i, &Time[i], &bnds_time[2 * i]);
    
    ierr |= cmor_axis(&axes_ids[0], "time1", "days since 2018", ntimes, &Time[0], 'd', 
                &bnds_time[0], 2, NULL);

    read_coords(&lat_coords[0], &lon_coords[0], &iplevs[0], &lat_bounds[0], &lon_bounds[0],
                lon, lat, lev);

    ierr |= cmor_axis(&axes_ids[1], "latitude", "degrees_north", lat, lat_coords, 'd', 
                lat_bounds, 2, "");
    ierr |= cmor_axis(&axes_ids[2], "longitude", "degrees_east", lon, lon_coords, 'd', 
                lon_bounds, 2, "");

    double alev_val[lev] = {0.996149986982346, 0.982649981975555, 
                        0.958955590856714, 0.92764596935028};
    double alev_bnds[lev + 1] = {1, 0.992299973964691, 0.97299998998642, 
                        0.944911191727007, 0.910380746973552};

    ierr |= cmor_axis(&axes_ids[3], (char *) "standard_hybrid_sigma", (char *) "", 
                lev, alev_val, 'd', alev_bnds, 1, NULL);

    double lambda550[1] = {550.};

    ierr |= cmor_axis(&axes_ids[4], "lambda550nm", "nm", 1, lambda550, 'd', 
                NULL, 0, "");

    double p0[1] = {101325.0};
    double a_val[4] = {0, 0, 36.0317993164062, 171.845031738281};
    double a_bnds[5] = {  0, 0, 0,  72.0635986328125, 271.62646484375};
    double b_val[4] = {0, 0, 36.0317993164062, 171.845031738281};
    double b_bnds[5] = {  0, 0,  0, 72.0635986328125, 271.62646484375};

    int lev_id_array[2];
    lev_id_array[0] = axes_ids[3];

    ierr |= cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "p0", (char *) "Pa", 0, 0, 
                'd', (void *) p0, NULL);
    ierr |= cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "b", (char *) "", 1, &lev_id_array[0], 
                'd', (void *) b_val, (void *) b_bnds);
    ierr |= cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "a", (char *) "", 1, &lev_id_array[0], 
                'd', (void *) a_val, (void *) a_bnds);
    ierr |= cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "ps1", (char *) "Pa", 3, axes_ids, 
                'd', NULL, NULL);

    ierr |= cmor_variable(&myvars[0], "ec550aer", "m-1", 5, axes_ids, 'd', NULL,
                &tolerance, "down", "ec550aer", "no history", "no future");

    for (i = 0; i < ntimes; i++) {
        read_3d_input_files(i, "CLOUD", &data3d[0], lat, lon, lev);
        read_2d_input_files(i, "PSURF", &data2d[0], lat, lon);

        ierr |= cmor_write(myvars[0], data3d, 'd', NULL, 1, NULL, NULL, NULL);
        ierr |= cmor_write(zfactor_id, data2d, 'd', NULL, 1, NULL, NULL, &myvars[0]);
    }
    ierr |= cmor_close_variable(myvars[0], returnvalue, NULL);

    printf("File: '%s' has been written\n", returnvalue);
    if(cmor_nerrors != 0) {
        printf("Error occured in test_cmor_depth_coord_half.\n");
        exit(1);
    }

    ierr |= cmor_close();

    return ierr;
}
