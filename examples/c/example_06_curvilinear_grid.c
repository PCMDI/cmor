#include "cmip7_c_common.h"

#include <stdio.h>

#define NX 4
#define NY 3
#define NVERTICES 4
#define NPARAMS 6
#define PARAM_LEN 32
#define UNIT_LEN 2

int main(int argc, char **argv) {
  const char *repo_root = argc > 1 ? argv[1] : ".";
  const char *output_dir = argc > 2 ? argv[2] : "examples/c/output";
  char input_path[CMIP7_PATH_MAX];
  char tables_path[CMIP7_PATH_MAX];
  char cell_measures[CMOR_MAX_STRING];
  char long_name[CMOR_MAX_STRING];
  char filename[CMOR_MAX_STRING];
  int file_action = CMOR_REPLACE;
  int exit_control = CMOR_EXIT_ON_MAJOR;
  int table_id, grid_table_id, x_id, y_id, time_id, grid_id, var_id;
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
  double time[CMIP7_NTIMES] = {15.5, 45.5};
  double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};
  char parameter_names[NPARAMS][PARAM_LEN] = {"standard_parallel1",
                                              "longitude_of_central_meridian",
                                              "latitude_of_projection_origin",
                                              "false_easting",
                                              "false_northing",
                                              "standard_parallel2"};
  char parameter_units[NPARAMS][UNIT_LEN] = {"", "", "", "", "", ""};
  double parameter_values[CMOR_MAX_GRID_ATTRIBUTES] = {-20.0, 175.0, 13.0,
                                                       8.0,   0.0,   20.0};
  float hfls[CMIP7_NTIMES * NY * NX];
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_06_input.json", "mon", "r1",
                              "f1", input_path, sizeof(input_path));

  cmor_setup(tables_path, &file_action, NULL, &exit_control, NULL, NULL);
  cmor_dataset_json(input_path);

  cmor_load_table("CMIP7_grids.json", &grid_table_id);
  cmor_set_table(grid_table_id);
  cmor_axis(&y_id, "y", "m", NY, y, 'd', y_bnds, 1, NULL);
  cmor_axis(&x_id, "x", "m", NX, x, 'd', x_bnds, 1, NULL);

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
  cmor_grid(&grid_id, 2, grid_axes, 'd', latitude, longitude, NVERTICES,
            latitude_vertices, longitude_vertices);
  cmor_set_grid_mapping(grid_id, "lambert_conformal_conic", NPARAMS,
                        &parameter_names[0][0], PARAM_LEN, parameter_values,
                        &parameter_units[0][0], UNIT_LEN);

  cmor_load_table("CMIP7_atmos.json", &table_id);
  cmor_axis(&time_id, "time", "days since 1979-01-01", CMIP7_NTIMES, time, 'd',
            time_bnds, 1, NULL);

  for (t = 0; t < CMIP7_NTIMES; ++t) {
    for (j = 0; j < NY; ++j) {
      for (i = 0; i < NX; ++i) {
        int idx = (t * NY + j) * NX + i;
        hfls[idx] = 80.0f + 2.0f * (float)i + 8.0f * (float)j + (float)t;
      }
    }
  }

  axes[0] = time_id;
  axes[1] = grid_id;
  cmor_variable(&var_id, "hfls_tavg-u-hxy-u", "W m-2", 2, axes, 'f', &missing,
                NULL, "up", NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "atmos", "hfls_tavg-u-hxy-u", "mon",
                          "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "atmos", "hfls_tavg-u-hxy-u",
                                   "mon", "glb", long_name,
                                   sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  cmor_write(var_id, hfls, 'f', NULL, CMIP7_NTIMES, NULL, NULL, NULL);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
