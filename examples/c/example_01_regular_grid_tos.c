#include "cmip7_c_common.h"

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    int lon_id, lat_id, time_id, var_id;
    int axes[3];
    float tos[CMIP7_NTIMES * CMIP7_NLAT * CMIP7_NLON] = {
        254.0895f, 258.4085f, CMIP7_MISSING_VALUE, 258.7101f,
        258.6680f, 258.2990f, CMIP7_MISSING_VALUE, 255.0432f,
        253.7254f, 251.2460f, CMIP7_MISSING_VALUE, 255.4808f,
        254.0995f, 258.5085f, CMIP7_MISSING_VALUE, 258.8101f,
        258.8680f, 258.4990f, CMIP7_MISSING_VALUE, 255.2432f,
        254.0254f, 251.5460f, CMIP7_MISSING_VALUE, 255.7808f};
    float missing = CMIP7_MISSING_VALUE;

    cmip7_prepare_cmor(argc, argv, "example_01_input.json", "mon", "r1",
                       "f1", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));
    cmip7_load_table("CMIP7_ocean.json");
    cmip7_define_lon_lat_time_axes("CMIP7_ocean.json", &lon_id, &lat_id,
                                   &time_id);

    axes[0] = time_id;
    axes[1] = lat_id;
    axes[2] = lon_id;
    cmip7_check_status("cmor_variable(tos)",
                       cmor_variable(&var_id, "tos_tavg-u-hxy-sea", "degC",
                                     3, axes, 'f', &missing, NULL, NULL, NULL,
                                     NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "ocean",
                                  "tos_tavg-u-hxy-sea", "mon", "glb");
    cmip7_check_status("cmor_write(tos)",
                       cmor_write(var_id, tos, 'f', NULL, CMIP7_NTIMES, NULL,
                                  NULL, NULL));
    cmip7_close_variable(var_id);
    return 0;
}
