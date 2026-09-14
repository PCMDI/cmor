#include "cmip7_c_common.h"

#include <stdio.h>

#define NSITES 126
#define NVERTICES 2

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
  int table_id, grid_table_id, time_id, site_id, height_id, grid_id, var_id;
  int grid_axes[1];
  int axes[3];
  int site, time_index;
  double time[CMIP7_NTIMES] = {15.5, 45.5};
  double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};
  double site_values[NSITES];
  double latitude[NSITES];
  double longitude[NSITES];
  double latitude_vertices[NSITES * NVERTICES];
  double longitude_vertices[NSITES * NVERTICES];
  double height = 2.0;
  float tas[CMIP7_NTIMES * NSITES];
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_08_input.json", "mon", "r1",
                              "f1", input_path, sizeof(input_path));

  cmor_setup(tables_path, &file_action, NULL, &exit_control, NULL, NULL);
  cmor_dataset_json(input_path);
  cmor_load_table("CMIP7_atmos.json", &table_id);

  cmor_axis(&time_id, "time1", "days since 1979-01-01", CMIP7_NTIMES, time,
            'd', time_bnds, 1, NULL);
  cmor_axis(&height_id, "height2m", "m", 1, &height, 'd', NULL, 0, NULL);

  for (site = 0; site < NSITES; ++site) {
    int vertex = site * NVERTICES;
    site_values[site] = (double)(site + 1);
    latitude[site] = -60.0 + 120.0 * (double)site / (double)(NSITES - 1);
    longitude[site] = 0.5 + 359.0 * (double)site / (double)(NSITES - 1);
    latitude_vertices[vertex] = latitude[site] - 0.25;
    latitude_vertices[vertex + 1] = latitude[site] + 0.25;
    longitude_vertices[vertex] = longitude[site] - 0.25;
    longitude_vertices[vertex + 1] = longitude[site] + 0.25;
  }
  cmor_axis(&site_id, "site", "1", NSITES, site_values, 'd', NULL, 0, NULL);

  cmor_load_table("CMIP7_grids.json", &grid_table_id);
  cmor_set_table(grid_table_id);
  grid_axes[0] = site_id;
  cmor_grid(&grid_id, 1, grid_axes, 'd', latitude, longitude, NVERTICES,
            latitude_vertices, longitude_vertices);

  cmor_load_table("CMIP7_atmos.json", &table_id);
  axes[0] = height_id;
  axes[1] = time_id;
  axes[2] = grid_id;
  cmor_variable(&var_id, "tas_tpt-h2m-hs-u", "K", 3, axes, 'f', &missing,
                NULL, NULL, NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "atmos", "tas_tpt-h2m-hs-u", "mon",
                          "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "atmos", "tas_tpt-h2m-hs-u",
                                   "mon", "glb", long_name,
                                   sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  for (time_index = 0; time_index < CMIP7_NTIMES; ++time_index) {
    for (site = 0; site < NSITES; ++site) {
      tas[time_index * NSITES + site] =
          275.0f + 15.0f * (float)(time_index * NSITES + site) /
                       (float)(CMIP7_NTIMES * NSITES - 1);
    }
  }
  cmor_write(var_id, tas, 'f', NULL, CMIP7_NTIMES, NULL, NULL, NULL);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
