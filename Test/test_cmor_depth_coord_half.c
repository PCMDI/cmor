#include <time.h>
#include <stdio.h>
#include <string.h>
#include "cmor.h"
#include <stdlib.h>

#define ntimes 2
#define nlat 1
#define nlon 1
#define nlev 4

int main()
{
    int myvars[1];
    int tables[1];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    char returnvalue[CMOR_MAX_STRING];
    int i, j, k;

    double Time[ntimes] = {0.5, 1.5};
    double bnds_time[ntimes][2] = {{0, 1}, {1, 2}};
    double lon_coords[nlat] = {0};
    double lat_coords[nlon] = {80};
    double lon_vertices[2 * nlat] = {-1, 1};
    double lat_vertices[2 * nlon] = {79, 81};
    double olev_val[nlev] = {5000., 3000., 2000., 1000.};
    double olev_bnds[nlev + 1] = {5000., 3000., 2000., 1000., 0};
    double zhalfo_data[ntimes] = {274., 274.};

    int exit_mode = CMOR_NORMAL;
    int cmor_mode = CMOR_REPLACE;
    int ierr = 0;
    
    printf("Testing depth_coord_half\n");

    ierr |= cmor_setup(NULL, &cmor_mode, NULL, &exit_mode, NULL, NULL);
    ierr |= cmor_dataset_json("Test/CMOR_input_example.json");
    ierr |= cmor_load_table("Tables/CMIP6_Omon.json", &tables[0]);

    ierr |= cmor_axis(&axes_ids[0], "time", "months since 1980", ntimes, &Time[0], 'd',
            &bnds_time[0], 2, NULL);
    ierr |= cmor_axis(&axes_ids[1], "latitude", "degrees_north", nlat, lat_coords, 'd', 
            lat_vertices, 2, "");
    ierr |= cmor_axis(&axes_ids[2], "longitude", "degrees_east", nlon, lon_coords, 'd', 
            lon_vertices, 2, "");
    ierr |= cmor_axis(&axes_ids[3], "depth_coord_half", (char *) "m", nlev, olev_val, 'd', 
            olev_bnds, 1, NULL);

    ierr |= cmor_variable(&myvars[0], "zhalfo", "m", 4, axes_ids, 'd', NULL,
            NULL, "", "zhalfo", "no history", "no future");

    ierr |= cmor_write(myvars[0], zhalfo_data, 'd', NULL, 0, NULL, NULL, NULL);

    ierr |= cmor_close_variable(myvars[0], returnvalue, NULL);

    printf("File: '%s' has been written\n", returnvalue);
    if(cmor_nerrors != 0) {
        printf("Error occured in test_cmor_depth_coord_half.\n");
        exit(1);
    }

    ierr |= cmor_close();
    
    return ierr;
}
