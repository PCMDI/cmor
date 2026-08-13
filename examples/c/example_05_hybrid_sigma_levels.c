#include "cmip7_c_common.h"

#include <stdio.h>

#define NLEV 5

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
  int table_id, lon_id, lat_id, time_id, lev_id, ps_id, zfactor_id, var_id;
  int axes[4];
  int lev_axis[1];
  int ps_axes[3];
  int i, j, k, t;
  double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
  double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
  double lon[CMIP7_NLON] = {0.0, 90.0, 180.0, 270.0};
  double lon_bnds[CMIP7_NLON + 1] = {-45.0, 45.0, 135.0, 225.0, 315.0};
  double time[CMIP7_NTIMES] = {15.5, 45.5};
  double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};
  double lev[NLEV] = {0.92, 0.72, 0.50, 0.30, 0.10};
  double lev_bnds[NLEV + 1] = {1.00, 0.83, 0.61, 0.40, 0.20, 0.00};
  double a_coeff[NLEV] = {0.12, 0.22, 0.30, 0.20, 0.10};
  double b_coeff[NLEV] = {0.80, 0.50, 0.20, 0.10, 0.00};
  double a_bnds[NLEV + 1] = {0.06, 0.18, 0.26, 0.25, 0.15, 0.00};
  double b_bnds[NLEV + 1] = {0.94, 0.65, 0.35, 0.15, 0.05, 0.00};
  double p0[1] = {100000.0};
  float cl[CMIP7_NTIMES * NLEV * CMIP7_NLAT * CMIP7_NLON];
  float ps[CMIP7_NTIMES * CMIP7_NLAT * CMIP7_NLON];
  float missing = CMIP7_MISSING_VALUE;

  snprintf(tables_path, sizeof(tables_path), "%s/cmip7-cmor-tables/tables",
           repo_root);
  cmip7_write_user_input_json(output_dir, "example_05_input.json", "mon", "r1",
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
  cmor_axis(&lev_id, "standard_hybrid_sigma", "1", NLEV, lev, 'd', lev_bnds, 1,
            NULL);

  lev_axis[0] = lev_id;
  cmor_zfactor(&zfactor_id, lev_id, "a", "", 1, lev_axis, 'd', a_coeff, a_bnds);
  cmor_zfactor(&zfactor_id, lev_id, "b", "", 1, lev_axis, 'd', b_coeff, b_bnds);
  cmor_zfactor(&zfactor_id, lev_id, "p0", "Pa", 0, NULL, 'd', p0, NULL);

  ps_axes[0] = time_id;
  ps_axes[1] = lat_id;
  ps_axes[2] = lon_id;
  cmor_zfactor(&ps_id, lev_id, "ps", "Pa", 3, ps_axes, 'f', NULL, NULL);

  for (t = 0; t < CMIP7_NTIMES; ++t) {
    for (j = 0; j < CMIP7_NLAT; ++j) {
      for (i = 0; i < CMIP7_NLON; ++i) {
        int idx = (t * CMIP7_NLAT + j) * CMIP7_NLON + i;
        ps[idx] = 97000.0f + 400.0f * (float)i + 1600.0f * (float)j +
                  100.0f * (float)t;
      }
    }
  }
  for (t = 0; t < CMIP7_NTIMES; ++t) {
    for (k = 0; k < NLEV; ++k) {
      for (j = 0; j < CMIP7_NLAT; ++j) {
        for (i = 0; i < CMIP7_NLON; ++i) {
          int idx = ((t * NLEV + k) * CMIP7_NLAT + j) * CMIP7_NLON + i;
          cl[idx] = 75.0f - 5.0f * (float)(k + 1) - 1.2f * (float)j +
                    0.4f * (float)i + 0.1f * (float)t;
        }
      }
    }
  }

  axes[0] = time_id;
  axes[1] = lev_id;
  axes[2] = lat_id;
  axes[3] = lon_id;
  cmor_variable(&var_id, "cl_tavg-al-hxy-u", "%", 4, axes, 'f', &missing, NULL,
                NULL, NULL, NULL, NULL);

  cmip7_get_cell_measures(tables_path, "atmos", "cl_tavg-al-hxy-u", "mon",
                          "glb", cell_measures, sizeof(cell_measures));
  cmor_set_variable_attribute(var_id, "cell_measures", 'c', cell_measures);
  if (cmip7_get_long_name_override(tables_path, "atmos", "cl_tavg-al-hxy-u",
                                   "mon", "glb", long_name,
                                   sizeof(long_name))) {
    cmor_set_variable_attribute(var_id, "long_name", 'c', long_name);
  }

  cmor_write(var_id, cl, 'f', NULL, CMIP7_NTIMES, NULL, NULL, NULL);
  cmor_write(ps_id, ps, 'f', NULL, CMIP7_NTIMES, NULL, NULL, &var_id);
  filename[0] = '\0';
  cmor_close_variable(var_id, filename, NULL);
  printf("%s\n", filename);
  cmor_close();
  return 0;
}
