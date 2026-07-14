#include "cmip7_c_common.h"

#include <stdio.h>

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
  int table_id, lon_id, lat_id, time_id, var_id;
  int axes[3];
  double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
  double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
  double lon[CMIP7_NLON] = {0.0, 90.0, 180.0, 270.0};
  double lon_bnds[CMIP7_NLON + 1] = {-45.0, 45.0, 135.0, 225.0, 315.0};
  double time[CMIP7_NTIMES] = {15.5, 45.5};
  double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};
  float tos[CMIP7_NTIMES * CMIP7_NLAT * CMIP7_NLON] = {
      254.0895f, 258.4085f, CMIP7_MISSING_VALUE, 258.7101f,
      258.6680f, 258.2990f, CMIP7_MISSING_VALUE, 255.0432f,
      253.7254f, 251.2460f, CMIP7_MISSING_VALUE, 255.4808f,
      254.0995f, 258.5085f, CMIP7_MISSING_VALUE, 258.8101f,
      258.8680f, 258.4990f, CMIP7_MISSING_VALUE, 255.2432f,
      254.0254f, 251.5460f, CMIP7_MISSING_VALUE, 255.7808f};
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_01_input.json", "mon", "r1",
                              "f1", input_path, sizeof(input_path));

  cmor_setup(tables_path, &file_action, NULL, &exit_control, NULL, NULL);
  cmor_dataset_json(input_path);
  cmor_load_table("CMIP7_ocean.json", &table_id);

  cmor_axis(&lon_id, "longitude", "degrees_east", CMIP7_NLON, lon, 'd',
            lon_bnds, 1, NULL);
  cmor_axis(&lat_id, "latitude", "degrees_north", CMIP7_NLAT, lat, 'd',
            lat_bnds, 1, NULL);
  cmor_axis(&time_id, "time", "days since 1979-01-01", CMIP7_NTIMES, time, 'd',
            time_bnds, 1, NULL);

  axes[0] = time_id;
  axes[1] = lat_id;
  axes[2] = lon_id;
  cmor_variable(&var_id, "tos_tavg-u-hxy-sea", "degC", 3, axes, 'f', &missing,
                NULL, NULL, NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "ocean", "tos_tavg-u-hxy-sea", "mon",
                          "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "ocean", "tos_tavg-u-hxy-sea",
                                   "mon", "glb", long_name,
                                   sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  cmor_write(var_id, tos, 'f', NULL, CMIP7_NTIMES, NULL, NULL, NULL);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
