#include "cmip7_c_common.h"

#include <stdio.h>

#define NBASIN 3

int main(int argc, char **argv) {
  const char *repo_root = argc > 1 ? argv[1] : ".";
  const char *output_dir = argc > 2 ? argv[2] : "examples/c/output";
  char input_path[CMIP7_PATH_MAX];
  char tables_path[CMIP7_PATH_MAX];
  char cell_measures[CMOR_MAX_STRING];
  char long_name[CMOR_MAX_STRING];
  char filename[CMOR_MAX_STRING];
  char basin_names[NBASIN][CMOR_MAX_STRING] = {
      "atlantic_arctic_ocean", "indian_pacific_ocean", "global_ocean"};
  int file_action = CMOR_REPLACE;
  int exit_control = CMOR_EXIT_ON_MAJOR;
  int table_id, lat_id, time_id, basin_id, var_id;
  int axes[3];
  double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
  double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
  double time[CMIP7_NTIMES] = {15.5, 45.5};
  double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};
  float heat_transport[CMIP7_NTIMES * NBASIN * CMIP7_NLAT] = {
      -80.0f,  -84.0f,  -88.0f, -100.0f, -104.0f, -76.0f,
      -120.0f, -92.0f,  -96.0f, -79.0f,  -83.0f,  -87.0f,
      -99.0f,  -103.0f, -75.0f, -107.0f, -111.0f, -115.0f};
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_04_input.json", "mon", "r1",
                              "f1", input_path, sizeof(input_path));

  cmor_setup(tables_path, &file_action, NULL, &exit_control, NULL, NULL);
  cmor_dataset_json(input_path);
  cmor_load_table("CMIP7_ocean.json", &table_id);

  cmor_axis(&lat_id, "latitude", "degrees_north", CMIP7_NLAT, lat, 'd',
            lat_bnds, 1, NULL);
  cmor_axis(&time_id, "time", "days since 1979-01-01", CMIP7_NTIMES, time, 'd',
            time_bnds, 1, NULL);
  cmor_axis(&basin_id, "basin", "", NBASIN, basin_names, 'c', NULL,
            CMOR_MAX_STRING, NULL);

  axes[0] = time_id;
  axes[1] = basin_id;
  axes[2] = lat_id;
  cmor_variable(&var_id, "htovgyre_tavg-u-hyb-sea", "W", 3, axes, 'f', &missing,
                NULL, NULL, NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "ocean", "htovgyre_tavg-u-hyb-sea",
                          "mon", "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "ocean",
                                   "htovgyre_tavg-u-hyb-sea", "mon", "glb",
                                   long_name, sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  cmor_write(var_id, heat_transport, 'f', NULL, CMIP7_NTIMES, NULL, NULL, NULL);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
