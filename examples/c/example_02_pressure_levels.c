#include "cmip7_c_common.h"

#define NPLEV 19

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    int lon_id, lat_id, time_id, plev_id, var_id;
    int axes[4];
    int i, j, k, t;
    double plev[NPLEV] = {100000.0, 92500.0, 85000.0, 70000.0, 60000.0,
                          50000.0,  40000.0, 30000.0, 25000.0, 20000.0,
                          15000.0,  10000.0, 7000.0,  5000.0,  3000.0,
                          2000.0,   1000.0,  500.0,   100.0};
    float ta[CMIP7_NTIMES * NPLEV * CMIP7_NLAT * CMIP7_NLON];
    float missing = CMIP7_MISSING_VALUE;

    cmip7_prepare_cmor(argc, argv, "example_02_input.json", "mon", "r1",
                       "f1", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));
    cmip7_load_table("CMIP7_atmos.json");
    cmip7_define_lon_lat_time_axes("CMIP7_atmos.json", &lon_id, &lat_id,
                                   &time_id);
    cmip7_check_status("cmor_axis(plev19)",
                       cmor_axis(&plev_id, "plev19", "Pa", NPLEV, plev, 'd',
                                 NULL, 0, NULL));

    for (t = 0; t < CMIP7_NTIMES; ++t) {
        for (k = 0; k < NPLEV; ++k) {
            for (j = 0; j < CMIP7_NLAT; ++j) {
                for (i = 0; i < CMIP7_NLON; ++i) {
                    int idx = ((t * NPLEV + k) * CMIP7_NLAT + j) *
                                  CMIP7_NLON +
                              i;
                    ta[idx] = 250.0f + 25.0f *
                                           (float)(i + 1 + 4 * (j + 1) +
                                                   12 * (k + 1) + 228 * t) /
                                           (float)(CMIP7_NLON * CMIP7_NLAT *
                                                   NPLEV * CMIP7_NTIMES);
                }
            }
        }
    }
    ta[0] = CMIP7_MISSING_VALUE;

    axes[0] = time_id;
    axes[1] = plev_id;
    axes[2] = lat_id;
    axes[3] = lon_id;
    cmip7_check_status("cmor_variable(ta)",
                       cmor_variable(&var_id, "ta_tavg-p19-hxy-air", "K", 4,
                                     axes, 'f', &missing, NULL, NULL, NULL,
                                     NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "atmos",
                                  "ta_tavg-p19-hxy-air", "mon", "glb");
    cmip7_check_status("cmor_write(ta)",
                       cmor_write(var_id, ta, 'f', NULL, CMIP7_NTIMES, NULL,
                                  NULL, NULL));
    cmip7_close_variable(var_id);
    return 0;
}
