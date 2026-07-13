#include "cmip7_c_common.h"

#define NBASIN 3

int main(int argc, char **argv)
{
    char repo_root[CMIP7_PATH_MAX];
    char output_dir[CMIP7_PATH_MAX];
    char tables_path[CMIP7_PATH_MAX];
    char basin_names[NBASIN][CMOR_MAX_STRING] = {
        "atlantic_arctic_ocean", "indian_pacific_ocean", "global_ocean"};
    int lat_id, time_id, basin_id, var_id;
    int axes[3];
    float heat_transport[CMIP7_NTIMES * NBASIN * CMIP7_NLAT] = {
        -80.0f,  -84.0f,  -88.0f,  -100.0f, -104.0f, -76.0f,
        -120.0f, -92.0f,  -96.0f,  -79.0f,  -83.0f,  -87.0f,
        -99.0f,  -103.0f, -75.0f,  -107.0f, -111.0f, -115.0f};
    float missing = CMIP7_MISSING_VALUE;

    cmip7_prepare_cmor(argc, argv, "example_04_input.json", "mon", "r1",
                       "f1", repo_root, sizeof(repo_root), output_dir,
                       sizeof(output_dir), tables_path, sizeof(tables_path));
    cmip7_load_table("CMIP7_ocean.json");
    cmip7_define_time_axis("CMIP7_ocean.json", &time_id);
    {
        double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
        double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
        cmip7_check_status("cmor_axis(latitude)",
                           cmor_axis(&lat_id, "latitude", "degrees_north",
                                     CMIP7_NLAT, lat, 'd', lat_bnds, 1,
                                     NULL));
    }
    cmip7_check_status("cmor_axis(basin)",
                       cmor_axis(&basin_id, "basin", "", NBASIN,
                                 basin_names, 'c', NULL, CMOR_MAX_STRING,
                                 NULL));

    axes[0] = time_id;
    axes[1] = basin_id;
    axes[2] = lat_id;
    cmip7_check_status("cmor_variable(htovgyre)",
                       cmor_variable(&var_id, "htovgyre_tavg-u-hyb-sea", "W",
                                     3, axes, 'f', &missing, NULL, NULL, NULL,
                                     NULL, NULL));
    cmip7_apply_variable_metadata(var_id, tables_path, "ocean",
                                  "htovgyre_tavg-u-hyb-sea", "mon", "glb");
    cmip7_check_status("cmor_write(htovgyre)",
                       cmor_write(var_id, heat_transport, 'f', NULL,
                                  CMIP7_NTIMES, NULL, NULL, NULL));
    cmip7_close_variable(var_id);
    return 0;
}
