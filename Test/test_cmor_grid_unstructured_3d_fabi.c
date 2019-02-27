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
#define   indi  10                /* number of i indices   */
#define   indj  10                /* number of j indices   */
#define   indk  10                /* number of k indices   */
#define   nvert 12
    double i_index[indi];
    double j_index[indj];
    double k_index[indk];
    double lon_coords[indi * indj * indk];
    double lat_coords[indi * indj * indk];
    double lon_vertices[indi * indj * indk * nvert];
    double lat_vertices[indi * indj * indk * nvert];

    double data3d[indi * indj * indk];

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
    for (k = 0; k < indk; k++) {
        k_index[k] = k;
        for (j = 0; j < indj; j++) {
            j_index[j] = j;
            for (i = 0; i < indi; i++) {
                i_index[i] = i;
                    lon_coords[k*indi*indj + j*indi + i] = lon0 + delta_lon * i;
                    lat_coords[k*indi*indj + j*indi + i] = lat0 + delta_lat * j;
                    /* vertices lon */
                    for (vert = 0; vert < nvert; vert++)
                    {
                        lon_vertices[(k*indi*indj + j*indi + i) * nvert + vert] =
                        lon_coords[k*indi*indj + j*indi + i];
                        /* vertices lat */
                        lat_vertices[(k*indi*indj + j*indi + i) * nvert + vert] =
                        lat_coords[k*indi*indj + j*indi + i];
                    }
            }
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
    ierr = cmor_axis(&myaxes[0], (char *) "i_index", (char *) "1", indi, (void *) i_index, 'd', 0,
                          0, NULL);
    ierr = cmor_axis(&myaxes[1], (char *) "j_index", (char *) "1", indj, (void *) j_index, 'd', 0,
                          0, NULL);
    ierr = cmor_axis(&myaxes[2], (char *) "k_index", (char *) "1", indk, (void *) k_index, 'd', 0,
                          0, NULL);
    printf("Test code: ok got axes id: %i for 'index'\n", myaxes[0]);
    printf("Test code: ok got axes id: %i for 'index'\n", myaxes[1]);
    printf("Test code: ok got axes id: %i for 'index'\n", myaxes[2]);

    /*now defines the grid */
    printf("going to grid stuff \n");
    ierr = cmor_grid(&mygrids[0], 3, myaxes, 'd', (void *) lat_coords, (void *) lon_coords, nvert,
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

        printf("Test code: 3d\n");
        read_3d_input_files(i, "T", &data3d[0], indi, indj, indk);
        //for(j=0;j<10;j++) printf("Test code: %i out of %i : %lf\n",j,9,data2d[j]);
        printf("var id: %i\n", myvars[0]);
        ierr = cmor_write(myvars[0], &data3d, 'd', NULL, 1, NULL, NULL, NULL);
    }
    printf("ok loop done\n");
    ierr = cmor_close();
    printf("Test code: done\n");
    return 0;
}
