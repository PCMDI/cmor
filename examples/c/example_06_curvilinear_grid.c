#include "cmip7_c_common.h"

#define NX 4
#define NY 3
#define NVERTICES 4
#define NPARAMS 6
#define PARAM_LEN 32
#define UNIT_LEN 2

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    int x_id, y_id, time_id, grid_id, grid_table_id, var_id;
    int grid_axes[2];
    int axes[2];
    int i, j, t;
    double x[NX] = {0.0, 10000.0, 20000.0, 30000.0};
    double y[NY] = {0.0, 10000.0, 20000.0};
    double x_bnds[NX + 1] = {-5000.0, 5000.0, 15000.0, 25000.0, 35000.0};
    double y_bnds[NY + 1] = {-5000.0, 5000.0, 15000.0, 25000.0};
    double latitude[NY * NX];
    double longitude[NY * NX];
    double latitude_vertices[NY * NX * NVERTICES];
    double longitude_vertices[NY * NX * NVERTICES];
    char parameter_names[NPARAMS][PARAM_LEN] = {
        "standard_parallel1", "longitude_of_central_meridian",
        "latitude_of_projection_origin", "false_easting", "false_northing",
        "standard_parallel2"};
    char parameter_units[NPARAMS][UNIT_LEN] = {"", "", "", "", "", ""};
    double parameter_values[CMOR_MAX_GRID_ATTRIBUTES] = {-20.0, 175.0, 13.0,
                                                         8.0,   0.0,   20.0};
    float hfls[CMIP7_NTIMES * NY * NX];
    float missing = CMIP7_MISSING_VALUE;

    cmip7_prepare_cmor(argc, argv, "example_06_input.json", "mon", "r1",
                       "f1", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));

    grid_table_id = cmip7_load_table("CMIP7_grids.json");
    cmip7_check_status("cmor_set_table", cmor_set_table(grid_table_id));
    cmip7_check_status("cmor_axis(y)",
                       cmor_axis(&y_id, "y", "m", NY, y, 'd', y_bnds, 1,
                                 NULL));
    cmip7_check_status("cmor_axis(x)",
                       cmor_axis(&x_id, "x", "m", NX, x, 'd', x_bnds, 1,
                                 NULL));

    for (j = 0; j < NY; ++j) {
        for (i = 0; i < NX; ++i) {
            int idx = j * NX + i;
            int vertex_idx = idx * NVERTICES;

            latitude[idx] = 10.0 * (double)(j + 1) - 2.0 * (double)i;
            longitude[idx] = 280.0 + 10.0 * (double)i + 2.0 * (double)j;
            latitude_vertices[vertex_idx + 0] = latitude[idx] - 5.0;
            latitude_vertices[vertex_idx + 1] = latitude[idx] - 4.0;
            latitude_vertices[vertex_idx + 2] = latitude[idx] + 5.0;
            latitude_vertices[vertex_idx + 3] = latitude[idx] + 4.0;
            longitude_vertices[vertex_idx + 0] = longitude[idx] - 5.0;
            longitude_vertices[vertex_idx + 1] = longitude[idx] + 5.0;
            longitude_vertices[vertex_idx + 2] = longitude[idx] + 5.0;
            longitude_vertices[vertex_idx + 3] = longitude[idx] - 5.0;
        }
    }

    grid_axes[0] = y_id;
    grid_axes[1] = x_id;
    cmip7_check_status("cmor_grid",
                       cmor_grid(&grid_id, 2, grid_axes, 'd', latitude,
                                 longitude, NVERTICES, latitude_vertices,
                                 longitude_vertices));
    cmip7_check_status("cmor_set_grid_mapping",
                       cmor_set_grid_mapping(grid_id,
                                             "lambert_conformal_conic",
                                             NPARAMS, &parameter_names[0][0],
                                             PARAM_LEN, parameter_values,
                                             &parameter_units[0][0],
                                             UNIT_LEN));

    cmip7_load_table("CMIP7_atmos.json");
    cmip7_define_time_axis("CMIP7_atmos.json", &time_id);
    for (t = 0; t < CMIP7_NTIMES; ++t) {
        for (j = 0; j < NY; ++j) {
            for (i = 0; i < NX; ++i) {
                int idx = (t * NY + j) * NX + i;
                hfls[idx] = 80.0f + 2.0f * (float)i + 8.0f * (float)j +
                            (float)t;
            }
        }
    }

    axes[0] = time_id;
    axes[1] = grid_id;
    cmip7_check_status("cmor_variable(hfls)",
                       cmor_variable(&var_id, "hfls_tavg-u-hxy-u", "W m-2",
                                     2, axes, 'f', &missing, NULL, "up",
                                     NULL, NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "atmos",
                                  "hfls_tavg-u-hxy-u", "mon", "glb");
    cmip7_check_status("cmor_write(hfls)",
                       cmor_write(var_id, hfls, 'f', NULL, CMIP7_NTIMES,
                                  NULL, NULL, NULL));
    cmip7_close_variable(var_id);
    return 0;
}
