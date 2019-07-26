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
#define   indi  5                /* number of i indices   */
#define   indj  4                /* number of j indices   */
#define   indk  3                /* number of k indices   */
#define   nvert 5
#define   nb 2

    double i_index[indi];
    double j_index[indj];
    double k_index[indk];
    double lon[indk][indj][indi];
    double lat[indk][indj][indi];
    double vertices_longitude[indk][indj][indi][nvert];
    double vertices_latitude[indk][indj][indi][nvert];

    double field[ntimes][indk][indj][indi];

    int myaxes[10];
    int mygrids[10];
    int myvars[10];
    int tables[4];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    int i, j, k, n, ierr;

    double Time[ntimes];
    double bnds_time[ntimes][nb];
    double tolerance = 1.e-4;
    char id[CMOR_MAX_STRING];
    double missing = 1.e20;

    int exit_mode;

    /* first construct grid lon/lat */
    for (k = 0; k < indk; k++) {
        k_index[k] = k;
        for (j = 0; j < indj; j++) {
            j_index[j] = j;
            for (i = 0; i < indi; i++) {
                i_index[i] = i;
                lat[k][j][i] = 82.5 - k*60 - j*15;
                lon[k][j][i] = i*72 + j*6 + k*18 + 36.0;

                vertices_latitude[k][j][i][0] = lat[k][j][i] - 7.5;
                vertices_latitude[k][j][i][1] = lat[k][j][i];
                vertices_latitude[k][j][i][2] = lat[k][j][i] + 7.5;
                vertices_latitude[k][j][i][3] = lat[k][j][i];
                vertices_latitude[k][j][i][4] = missing;

                vertices_longitude[k][j][i][0] = lon[k][j][i];
                vertices_longitude[k][j][i][1] = lon[k][j][i] + 36.0;
                vertices_longitude[k][j][i][2] = lon[k][j][i];
                vertices_longitude[k][j][i][3] = lon[k][j][i] - 36.0;
                vertices_longitude[k][j][i][4] = missing;
            }
        }
    }

    vertices_latitude[0][0][0][4] =  81.0;
    vertices_longitude[0][0][0][4] =  1.0;
    vertices_latitude[indk-1][indj-1][indi-1][4] = -81.0;
    vertices_longitude[indk-1][indj-1][indi-1][4] = 341.0;

    for (n = 0; n < ntimes; n++) {
        for (k = 0; k < indk; k++) {
            for (j = 0; j < indj; j++) {
                for (i = 0; i < indi; i++) {
                    field[n][k][j][i] = i + 10*j + 100*k + 1000*n;
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
    ierr = cmor_grid(&mygrids[0], 3, myaxes, 'd', (void *) lat, (void *) lon, nvert,
                          (void *) vertices_latitude, (void *) vertices_longitude);

    for (i = 0; i < cmor_grids[0].ndims; i++) {
        printf("Dim : %i the grid has the follwoing axes on itself: %i (%s)\n",
               i, cmor_grids[0].axes_ids[i],
               cmor_axes[cmor_grids[0].axes_ids[i]].id);
    }

    /* ok sets back the vars table */
    cmor_set_table(tables[1]);

    for (i = 0; i < ntimes; i++){
        read_time(i, &Time[i], &bnds_time[i]);
    }
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
        printf("var id: %i\n", myvars[0]);
        ierr = cmor_write(myvars[0], &field[i], 'd', NULL, 1, NULL, NULL, NULL);
    }
    printf("ok loop done\n");
    ierr = cmor_close();
    printf("Test code: done\n");
    return 0;
}
