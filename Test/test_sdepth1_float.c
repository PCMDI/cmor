#include <time.h>
#include <stdio.h>
#include<string.h>
#include "cmor.h"
#include <stdlib.h>

void read_coords(alats, alons, bnds_lat, bnds_lon, lon, lat)
double *alats, *alons;
double *bnds_lat, *bnds_lon;
int lon, lat;
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
}

void read_time(it, time, time_bnds)
int it;
double time[];
double time_bnds[];
{
    time[0] = it;
    time_bnds[0] = it;
    time_bnds[1] = it + 1;
}

#include "reader_2D_3D.h"

int main()
{
#define   ntimes  2             /* number of time samples to process */
#define   lon  4                /* number of longitude grid cells   */
#define   lat  3                /* number of latitude grid cells */

    double data2d[lat * lon];
    double alats[lat];
    double alons[lon];
    double Time[ntimes];
    double bnds_time[ntimes * 2];
    double bnds_lat[lat * 2];
    double bnds_lon[lon * 2];

    // Set 'sdepth1' as its valid_max value but as a float instead of a double.
    float sdepth1[1]  = {0.1f};
    float bnds_dep[2] = {0.0f, 0.1f};

    double dtmp, dtmp2;

    int m, i, ierr, j;
    int myaxes[4];
    int myvars[1];
    int tables[1];

    m = CMOR_EXIT_ON_MAJOR;
    j = CMOR_REPLACE_4;
    i = 1;
    printf("ok mode is:%i\n", m);
    ierr = cmor_setup(NULL, &j, NULL, &m, NULL, &i);

    read_coords(&alats[0], &alons[0], &bnds_lat[0], &bnds_lon[0],
                lon, lat);

    ierr = cmor_dataset_json("Test/CMOR_input_example.json");
    ierr = cmor_load_table("Tables/CMIP6_Lmon.json", &tables[0]);

    cmor_set_table(tables[0]);

    for (i = 0; i < ntimes; i++) {
        read_time(i, &Time[i], &bnds_time[i]);
    }

    ierr = cmor_axis(&myaxes[0], "time", "months since 1980", 
                    ntimes, &Time[0], 'd', &bnds_time[0], 2, "1 month");
    ierr = cmor_axis(&myaxes[1], "latitude", "degrees_north", 
                    lat, &alats, 'd', &bnds_lat, 2, "");
    ierr = cmor_axis(&myaxes[2], "longitude", "degrees_east", 
                    lon, &alons, 'd', &bnds_lon, 2, "");
    ierr = cmor_axis(&myaxes[3], "sdepth1", "m", 
                    1, &sdepth1, 'f', &bnds_dep, 2, "");

    dtmp = -999;
    dtmp2 = 1.e-4;

    ierr =
      cmor_variable(&myvars[0], "mrsos", "kg m-2", 4, myaxes, 'd', &dtmp,
                    &dtmp2, "", "SOIL_WET", "no history",
                    "no future");

    for (i = 0; i < ntimes; i++) {
        read_2d_input_files(i, "SOIL_WET", &data2d, lat, lon);
        ierr = cmor_write(myvars[0], &data2d, 'd', NULL, 1, NULL, NULL, NULL);
        if (ierr)
            return (1);

    }
    
    ierr = cmor_close();
    return (0);
}
