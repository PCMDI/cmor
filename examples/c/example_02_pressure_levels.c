#include "cmip7_c_common.h"

#include <stdio.h>

#define NPLEV 19

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
  int table_id, lon_id, lat_id, time_id, plev_id, var_id;
  int axes[4];
  int i, j, k, t;
  double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
  double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
  double lon[CMIP7_NLON] = {0.0, 90.0, 180.0, 270.0};
  double lon_bnds[CMIP7_NLON + 1] = {-45.0, 45.0, 135.0, 225.0, 315.0};
  double time[CMIP7_NTIMES] = {15.5, 45.5};
  double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};
  double plev[NPLEV] = {100000.0, 92500.0, 85000.0, 70000.0, 60000.0,
                        50000.0,  40000.0, 30000.0, 25000.0, 20000.0,
                        15000.0,  10000.0, 7000.0,  5000.0,  3000.0,
                        2000.0,   1000.0,  500.0,   100.0};
  float ta[CMIP7_NTIMES * NPLEV * CMIP7_NLAT * CMIP7_NLON];
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_02_input.json", "mon", "r1",
                              "f1", input_path, sizeof(input_path));

  cmor_setup(tables_path, &file_action, NULL, &exit_control, NULL, NULL);
  cmor_dataset_json(input_path);
  cmor_load_table("CMIP7_atmos.json", &table_id);

  cmor_axis(&lon_id, "longitude", "degrees_east", CMIP7_NLON, lon, 'd',
            lon_bnds, 1, NULL);
  cmor_axis(&lat_id, "latitude", "degrees_north", CMIP7_NLAT, lat, 'd',
            lat_bnds, 1, NULL);
  cmor_axis(&time_id, "time", "days since 1979-01-01", CMIP7_NTIMES, time, 'd',
            time_bnds, 1, NULL);
  cmor_axis(&plev_id, "plev19", "Pa", NPLEV, plev, 'd', NULL, 0, NULL);

  for (t = 0; t < CMIP7_NTIMES; ++t) {
    for (k = 0; k < NPLEV; ++k) {
      for (j = 0; j < CMIP7_NLAT; ++j) {
        for (i = 0; i < CMIP7_NLON; ++i) {
          int idx = ((t * NPLEV + k) * CMIP7_NLAT + j) * CMIP7_NLON + i;
          ta[idx] = 250.0f +
                    25.0f *
                        (float)(i + 1 + 4 * (j + 1) + 12 * (k + 1) + 228 * t) /
                        (float)(CMIP7_NLON * CMIP7_NLAT * NPLEV * CMIP7_NTIMES);
        }
      }
    }
  }
  ta[0] = CMIP7_MISSING_VALUE;

  axes[0] = time_id;
  axes[1] = plev_id;
  axes[2] = lat_id;
  axes[3] = lon_id;
  cmor_variable(&var_id, "ta_tavg-p19-hxy-air", "K", 4, axes, 'f', &missing,
                NULL, NULL, NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "atmos", "ta_tavg-p19-hxy-air", "mon",
                          "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "atmos", "ta_tavg-p19-hxy-air",
                                   "mon", "glb", long_name,
                                   sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  cmor_write(var_id, ta, 'f', NULL, CMIP7_NTIMES, NULL, NULL, NULL);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
