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
  int table_id, lon_id, lat_id, var_id;
  int axes[2];
  double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
  double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
  double lon[CMIP7_NLON] = {0.0, 90.0, 180.0, 270.0};
  double lon_bnds[CMIP7_NLON + 1] = {-45.0, 45.0, 135.0, 225.0, 315.0};
  float rootd[CMIP7_NLAT * CMIP7_NLON] = {0.50f,
                                          0.45f,
                                          CMIP7_MISSING_VALUE,
                                          0.55f,
                                          0.60f,
                                          0.60f,
                                          CMIP7_MISSING_VALUE,
                                          0.55f,
                                          CMIP7_MISSING_VALUE,
                                          0.45f,
                                          0.50f,
                                          0.50f};
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_07_input.json", "fx", "r1",
                              "f1", input_path, sizeof(input_path));

  cmor_setup(tables_path, &file_action, NULL, &exit_control, NULL, NULL);
  cmor_dataset_json(input_path);
  cmor_load_table("CMIP7_land.json", &table_id);

  cmor_axis(&lon_id, "longitude", "degrees_east", CMIP7_NLON, lon, 'd',
            lon_bnds, 1, NULL);
  cmor_axis(&lat_id, "latitude", "degrees_north", CMIP7_NLAT, lat, 'd',
            lat_bnds, 1, NULL);

  axes[0] = lat_id;
  axes[1] = lon_id;
  cmor_variable(&var_id, "rootd_ti-u-hxy-lnd", "m", 2, axes, 'f', &missing,
                NULL, NULL, NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "land", "rootd_ti-u-hxy-lnd", "fx",
                          "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "land", "rootd_ti-u-hxy-lnd",
                                   "fx", "glb", long_name, sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  cmor_write(var_id, rootd, 'f', NULL, 0, NULL, NULL, NULL);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
