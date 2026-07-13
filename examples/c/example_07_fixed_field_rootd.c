#include "cmip7_c_common.h"

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    int lon_id, lat_id, var_id;
    int axes[2];
    float rootd[CMIP7_NLAT * CMIP7_NLON] = {
        0.50f, 0.45f, CMIP7_MISSING_VALUE, 0.55f,
        0.60f, 0.60f, CMIP7_MISSING_VALUE, 0.55f,
        CMIP7_MISSING_VALUE, 0.45f, 0.50f, 0.50f};
    float missing = CMIP7_MISSING_VALUE;

    cmip7_prepare_cmor(argc, argv, "example_07_input.json", "fx", "r1",
                       "f1", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));
    cmip7_load_table("CMIP7_land.json");
    cmip7_define_lon_lat_axes("CMIP7_land.json", &lon_id, &lat_id);

    axes[0] = lat_id;
    axes[1] = lon_id;
    cmip7_check_status("cmor_variable(rootd)",
                       cmor_variable(&var_id, "rootd_ti-u-hxy-lnd", "m", 2,
                                     axes, 'f', &missing, NULL, NULL, NULL,
                                     NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "land",
                                  "rootd_ti-u-hxy-lnd", "fx", "glb");
    cmip7_check_status("cmor_write(rootd)",
                       cmor_write(var_id, rootd, 'f', NULL, 0, NULL, NULL,
                                  NULL));
    cmip7_close_variable(var_id);
    return 0;
}
