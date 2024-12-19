#include <time.h>
#include <stdio.h>
#include<string.h>
#include "cmor.h"
#include <stdlib.h>
#include <math.h>

void read_time(int it, double *time, double *time_bnds)
{
    time[0] = (it - 0.5) * 30.;
    time_bnds[0] = (it - 1) * 30.;
    time_bnds[1] = it * 30.;

    time[0] = it;
    time_bnds[0] = it;
    time_bnds[1] = it + 1;

}

#include "reader_2D_3D.h"

void read_coords(double *alats, double *alons, int *plevs,
                 double *bnds_lat, double *bnds_lon,
                 int lon, int lat, int lev)
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

    /*   dimension parameters: */
    /* --------------------------------- */
#define   ntimes  2            /* number of time samples to process */
#define   lon  3                /* number of longitude grid cells   */
#define   lat  4                /* number of latitude grid cells */
#define   lev  4                /* number of standard pressure levels */
#define nvert 2

void loopRoutine(char *times, char *returnvalue)
{
    int iplevs[lev];
    double lon_coords[lon];
    double lat_coords[lat];
    double lon_vertices[lon * nvert];
    double lat_vertices[lat * nvert];

    double data2d[lat * lon];
    double data3d[lev * lat * lon];

    int myvars[10];
    int zfactor_id;
    int tables[4];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    int i, j, k, ierr;

    double Time[ntimes];
    double bnds_time[ntimes * 2];
    double tolerance = 1.e-4;
    double lon0 = 280.;
    double lat0 = 0.;
    double delta_lon = 10.;
    double delta_lat = 10.;
    char id[CMOR_MAX_STRING];
    double tmpf = 0.;

    int exit_mode = CMOR_EXIT_ON_MAJOR;
    j = ( times ) ? CMOR_APPEND : CMOR_REPLACE;
    printf("Test code: ok init cmor, %i\n", exit_mode);
    ierr = cmor_setup(NULL, &j, NULL, &exit_mode, NULL, NULL);
    printf("Test code: ok init cmor\n");
    int tmpmo[12];
    ierr = cmor_dataset_json("Test/CMOR_input_example.json");
    printf("Test code: ok load cmor table(s)\n");
    ierr = cmor_load_table("Tables/CMIP6_Amon.json", &tables[1]);
    printf("Test code: ok load cmor table(s)\n");

    k = ( times ) ? 1 : 0 ;
    for (i = k*ntimes; i < k*ntimes+ntimes; i++)
        read_time(i, &Time[i-k*ntimes], &bnds_time[2 * (i-k*ntimes)]);
    
    if(times)
        ierr = cmor_axis(&axes_ids[0], "time", "months since 1980", 0, NULL, 'd',
                NULL, 0, NULL);
    else
        ierr = cmor_axis(&axes_ids[0], "time", "months since 1980", ntimes, &Time[0], 'd',
                &bnds_time[0], 2, NULL);


    read_coords(&lat_coords[0], &lon_coords[0], &iplevs[0], &lat_vertices[0], &lon_vertices[0],
                lon, lat, lev);
    ierr =
      cmor_axis(&axes_ids[1], "latitude", "degrees_north", lat, &lat_coords, 'd', &lat_vertices, 2,
                "");
    ierr =
      cmor_axis(&axes_ids[2], "longitude", "degrees_east", lon, &lon_coords, 'd', &lon_vertices, 2,
                "");

    double alev_val[4] = {0.996149986982346, 0.982649981975555, 0.958955590856714, 
    0.92764596935028};
    double alev_bnds[5] = {1, 0.992299973964691, 0.97299998998642, 0.944911191727007, 0.910380746973552};

    ierr = cmor_axis(&axes_ids[3], (char *) "alternate_hybrid_sigma", (char *) "", lev,
                      alev_val, 'd', alev_bnds, 1, NULL);

    double p0[1] = {101325.0};
    double ap_val[4] = {0, 0, 36.0317993164062, 171.845031738281};
    double ap_bnds[5] = {  0, 0, 0,  72.0635986328125, 271.62646484375};
    double b_val[4] = {0, 0, 36.0317993164062, 171.845031738281};
    double b_bnds[5] = {  0, 0,  0, 72.0635986328125, 271.62646484375};

    int lev_id_array[2];
    lev_id_array[0] = axes_ids[3];

    ierr = cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "p0", (char *) "Pa", 0, 0, 'd', (void *) p0, NULL);
    ierr = cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "b", (char *) "", 1, &lev_id_array[0], 'd',
                                 (void *) b_val, (void *) b_bnds);
    ierr = cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "ap", (char *) "Pa", 1, &lev_id_array[0], 'd',
                                 (void *) ap_val, (void *) ap_bnds);
    ierr = cmor_zfactor(&zfactor_id, axes_ids[3], (char *) "ps", (char *) "Pa", 3,
                                 axes_ids, 'd', NULL, NULL);

    ierr =
      cmor_variable(&myvars[0], "cli", "kg kg-1", 4, axes_ids, 'd', NULL,
                    &tolerance, "", "cli", "no history", "no future");

    for (i = 0; i < ntimes; i++) {
        printf("Test code: writing time: %i of %i\n", i + 1, ntimes);

        printf("Test code: 3d\n");
        read_3d_input_files(i, "CLOUD", data3d, lat, lon, lev);
        read_2d_input_files(i, "PSURF", data2d, lat, lon);
        //for(j=0;j<10;j++) printf("Test code: %i out of %i : %lf\n",j,9,data2d[j]);
        printf("var id: %i\n", myvars[0]);
        if(times) {
            ierr = cmor_write(myvars[0], data3d, 'd', times, 1, &Time[i], &bnds_time[i*2], NULL);
            ierr = cmor_write(zfactor_id, data2d, 'd', times, 1, &Time[i], &bnds_time[i*2], &myvars[0]);
        } else {
            ierr = cmor_write(myvars[0], data3d, 'd', times, 1, NULL, NULL, NULL);
            ierr = cmor_write(zfactor_id, data2d, 'd', times, 1, NULL, NULL, &myvars[0]);
        }
    }
    printf("ok loop done\n");
    ierr = cmor_close_variable(myvars[0], returnvalue, NULL);
}

int main()
{
  char returnvalue[CMOR_MAX_STRING];
  loopRoutine(NULL, returnvalue);
  printf("File: '%s' has been written", returnvalue);
  loopRoutine(returnvalue, returnvalue);
  printf("File: '%s' has been written", returnvalue);
  printf("Test code: done\n");
}
