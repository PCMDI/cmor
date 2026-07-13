#include "cmip7_c_common.h"

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    int lon_id, lat_id, time_id, height_id, var_id;
    int axes[3];
    double height = 2.0;
    float tas[CMIP7_NTIMES * CMIP7_NLAT * CMIP7_NLON] = {
        254.0895f, 258.4085f, 250.5549f, 258.7101f,
        258.6680f, 258.2990f, 252.1237f, 255.0432f,
        253.7254f, 251.2460f, 254.3168f, 255.4808f,
        259.7908f, 252.2754f, 257.1892f, 253.3132f,
        253.8823f, 253.4698f, 253.5381f, 254.9730f,
        256.1002f, 251.8168f, 259.3698f, 250.2994f};
    float missing = CMIP7_MISSING_VALUE;

    cmip7_prepare_cmor(argc, argv, "example_03_input.json", "mon", "r9",
                       "f2", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));
    cmip7_load_table("CMIP7_atmos.json");
    cmip7_define_lon_lat_time_axes("CMIP7_atmos.json", &lon_id, &lat_id,
                                   &time_id);
    cmip7_check_status("cmor_axis(height2m)",
                       cmor_axis(&height_id, "height2m", "m", 1, &height,
                                 'd', NULL, 0, NULL));

    axes[0] = time_id;
    axes[1] = lat_id;
    axes[2] = lon_id;
    cmip7_check_status("cmor_variable(tas)",
                       cmor_variable(&var_id, "tas_tavg-h2m-hxy-u", "K", 3,
                                     axes, 'f', &missing, NULL, NULL, NULL,
                                     NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "atmos",
                                  "tas_tavg-h2m-hxy-u", "mon", "glb");
    cmip7_check_status("cmor_write(tas)",
                       cmor_write(var_id, tas, 'f', NULL, CMIP7_NTIMES, NULL,
                                  NULL, NULL));
    cmip7_close_variable(var_id);
    return 0;
}
