#include "cmip7_c_common.h"

#define NLEV 5

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    int lon_id, lat_id, time_id, lev_id, ps_id, var_id;
    int axes[4];
    int lev_axis[1];
    int ps_axes[3];
    int i, j, k, t;
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

    cmip7_prepare_cmor(argc, argv, "example_05_input.json", "mon", "r1",
                       "f1", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));
    cmip7_load_table("CMIP7_atmos.json");
    cmip7_define_lon_lat_time_axes("CMIP7_atmos.json", &lon_id, &lat_id,
                                   &time_id);
    cmip7_check_status("cmor_axis(standard_hybrid_sigma)",
                       cmor_axis(&lev_id, "standard_hybrid_sigma", "1",
                                 NLEV, lev, 'd', lev_bnds, 1, NULL));

    lev_axis[0] = lev_id;
    cmip7_check_status("cmor_zfactor(a)",
                       cmor_zfactor(&ps_id, lev_id, "a", "", 1, lev_axis,
                                    'd', a_coeff, a_bnds));
    cmip7_check_status("cmor_zfactor(b)",
                       cmor_zfactor(&ps_id, lev_id, "b", "", 1, lev_axis,
                                    'd', b_coeff, b_bnds));
    cmip7_check_status("cmor_zfactor(p0)",
                       cmor_zfactor(&ps_id, lev_id, "p0", "Pa", 0, NULL,
                                    'd', p0, NULL));

    ps_axes[0] = time_id;
    ps_axes[1] = lat_id;
    ps_axes[2] = lon_id;
    cmip7_check_status("cmor_zfactor(ps)",
                       cmor_zfactor(&ps_id, lev_id, "ps", "Pa", 3,
                                    ps_axes, 'f', NULL, NULL));

    for (t = 0; t < CMIP7_NTIMES; ++t) {
        for (j = 0; j < CMIP7_NLAT; ++j) {
            for (i = 0; i < CMIP7_NLON; ++i) {
                int idx = (t * CMIP7_NLAT + j) * CMIP7_NLON + i;
                ps[idx] = 97000.0f + 400.0f * (float)i +
                          1600.0f * (float)j + 100.0f * (float)t;
            }
        }
    }
    for (t = 0; t < CMIP7_NTIMES; ++t) {
        for (k = 0; k < NLEV; ++k) {
            for (j = 0; j < CMIP7_NLAT; ++j) {
                for (i = 0; i < CMIP7_NLON; ++i) {
                    int idx = ((t * NLEV + k) * CMIP7_NLAT + j) *
                                  CMIP7_NLON +
                              i;
                    cl[idx] = 75.0f - 5.0f * (float)(k + 1) -
                              1.2f * (float)j + 0.4f * (float)i +
                              0.1f * (float)t;
                }
            }
        }
    }

    axes[0] = time_id;
    axes[1] = lev_id;
    axes[2] = lat_id;
    axes[3] = lon_id;
    cmip7_check_status("cmor_variable(cl)",
                       cmor_variable(&var_id, "cl_tavg-al-hxy-u", "%", 4,
                                     axes, 'f', &missing, NULL, NULL, NULL,
                                     NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "atmos",
                                  "cl_tavg-al-hxy-u", "mon", "glb");
    cmip7_check_status("cmor_write(cl)",
                       cmor_write(var_id, cl, 'f', NULL, CMIP7_NTIMES, NULL,
                                  NULL, NULL));
    cmip7_check_status("cmor_write(ps)",
                       cmor_write(ps_id, ps, 'f', NULL, CMIP7_NTIMES, NULL,
                                  NULL, &var_id));
    cmip7_close_variable(var_id);
    return 0;
}
