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

int main()
{

    /*   dimension parameters: */
    /* --------------------------------- */
    
#define   ntimes  2             /* number of time samples to process */
#define   ind  10                /* number of indices   */
#define   lev  2                /* number of ocean depth levels */
#define   nvert 12
    double index[ind];
    double lon_coords[ind];
    double lat_coords[ind];
    double lon_vertices[ind * nvert];
    double lat_vertices[ind * nvert];

    double data2d[ind];
    double data3d[lev * ind];

    int myaxes[10];
    int mygrids[10];
    int myvars[10];
    int tables[4];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    int i, j, vert, k, ierr;

    double Time[ntimes];
    double bnds_time[ntimes * 2];
    double tolerance = 1.e-4;
    double lon0 = 0.;
    double lat0 = 0.;
    double delta_lon = 36.;
    double delta_lat = 18.;
    char id[CMOR_MAX_STRING];
    double tmpf = 0.;

    int exit_mode;
    /* first construct grid lon/lat */
    for (j = 0; j < ind; j++) {
            index[j] = j;
            lon_coords[j] = lon0 + delta_lon * (j);
            lat_coords[j] = lat0 + delta_lat * (j);
            /* vertices lon */
            for (vert = 0; vert < nvert; vert++)
              {
                lon_vertices[j * nvert + vert] =
                    lon_coords[j];
                /* vertices lat */
                lat_vertices[j * nvert + vert] =
                    lat_coords[j];
              }
        }

    exit_mode = CMOR_EXIT_ON_MAJOR;
    j = CMOR_REPLACE;
    printf("Test code: ok init cmor, %i\n", exit_mode);
    ierr = cmor_setup(NULL, &j, NULL, &exit_mode, NULL, NULL);
    printf("Test code: ok init cmor\n");
    int tmpmo[12];
    ierr = cmor_dataset_json("Test/CMOR_input_example.json");
    printf("Test code: ok load cmor table(s)\n");
    ierr = cmor_load_table("Tables/CMIP6_Amon.json", &tables[1]);
    printf("Test code: ok load cmor table(s)\n");
    //ierr = cmor_load_table("Test/IPCC_test_table_Grids",&tables[0]);
    ierr = cmor_load_table("Tables/CMIP6_grids.json", &tables[0]);
    printf("Test code: ok load cmor table(s)\n");
    ierr = cmor_set_table(tables[0]);

    /* first define grid axes (x/y/rlon/rlat,etc... */
    ierr = cmor_axis(&myaxes[0], (char *) "i_index", (char *) "1", ind, (void *) index, 'd', 0,
                          0, NULL);
    printf("Test code: ok got axes id: %i for 'index'\n", myaxes[0]);

    /*now defines the grid */
    printf("going to grid stuff \n");
    ierr = cmor_grid(&mygrids[0], 1, myaxes, 'd', (void *) lat_coords, (void *) lon_coords, nvert,
                          (void *) lat_vertices, (void *) lon_vertices);

    for (i = 0; i < cmor_grids[0].ndims; i++) {
        printf("Dim : %i the grid has the follwoing axes on itself: %i (%s)\n",
               i, cmor_grids[0].axes_ids[i],
               cmor_axes[cmor_grids[0].axes_ids[i]].id);
    }

    /* ok sets back the vars table */
    cmor_set_table(tables[1]);

    for (i = 0; i < ntimes; i++)
        read_time(i, &Time[i], &bnds_time[2 * i]);
    ierr =
      cmor_axis(&axes_ids[0], "time", "months since 1980", 2, &Time[0], 'd',
                &bnds_time[0], 2, NULL);

    printf("time axis id: %i\n", axes_ids[0]);
    axes_ids[1] = mygrids[0];   /*grid */

    printf("Test code: sending axes_ids: %i %i\n", axes_ids[0], axes_ids[1]);

    ierr =
      cmor_variable(&myvars[0], "hfls", "W m-2", 2, axes_ids, 'd', NULL,
                    &tolerance, "down", "HFLS", "no history", "no future");

    for (i = 0; i < ntimes; i++) {
        printf("Test code: writing time: %i of %i\n", i + 1, ntimes);

        printf("Test code: 2d\n");
        read_2d_input_files(i, "LATENT", &data2d[0], ind, 1);
        //for(j=0;j<10;j++) printf("Test code: %i out of %i : %lf\n",j,9,data2d[j]);
        printf("var id: %i\n", myvars[0]);
        ierr = cmor_write(myvars[0], &data2d, 'd', NULL, 1, NULL, NULL, NULL);
    }
    printf("ok loop done\n");
    ierr = cmor_close();
    printf("Test code: done\n");
    return 0;
}
