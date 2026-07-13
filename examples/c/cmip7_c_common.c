#include "cmip7_c_common.h"

#include <errno.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static void cmip7_copy_string(char *out, size_t out_size, const char *value)
{
    if (out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", value);
}

void cmip7_join_path(char *out, size_t out_size, const char *left,
                     const char *right)
{
    size_t left_len = strlen(left);

    if (left_len > 0 && left[left_len - 1] == '/') {
        snprintf(out, out_size, "%s%s", left, right);
    } else {
        snprintf(out, out_size, "%s/%s", left, right);
    }
}

static void cmip7_ensure_directory(const char *path)
{
    if (mkdir(path, 0775) != 0 && errno != EEXIST) {
        fprintf(stderr, "Could not create directory %s: %s\n", path,
                strerror(errno));
        exit(1);
    }
}

void cmip7_check_status(const char *call_name, int ierr)
{
    if (ierr != 0) {
        fprintf(stderr, "%s failed with status %d\n", call_name, ierr);
        exit(1);
    }
}

void cmip7_get_example_args(int argc, char **argv, char *repo_root,
                            size_t repo_root_size, char *output_dir,
                            size_t output_dir_size)
{
    cmip7_copy_string(repo_root, repo_root_size, argc > 1 ? argv[1] : ".");
    cmip7_copy_string(output_dir, output_dir_size,
                      argc > 2 ? argv[2] : "examples/c/output");
}

static void cmip7_write_input_json(const char *input_path,
                                   const char *output_dir,
                                   const char *frequency,
                                   const char *realization_index,
                                   const char *forcing_index)
{
    FILE *file = fopen(input_path, "w");

    if (file == NULL) {
        fprintf(stderr, "Could not write %s: %s\n", input_path,
                strerror(errno));
        exit(1);
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"_AXIS_ENTRY_FILE\": \"CMIP7_coordinate.json\",\n");
    fprintf(file, "  \"_FORMULA_VAR_FILE\": \"CMIP7_formula_terms.json\",\n");
    fprintf(file, "  \"_cmip7_option\": 1,\n");
    fprintf(file,
            "  \"_controlled_vocabulary_file\": "
            "\"../tables-cvs/cmor-cvs.json\",\n");
    fprintf(file, "  \"activity_id\": \"CMIP\",\n");
    fprintf(file, "  \"archive_id\": \"WCRP\",\n");
    fprintf(file, "  \"calendar\": \"360_day\",\n");
    fprintf(file, "  \"experiment_id\": \"amip\",\n");
    fprintf(file, "  \"forcing_index\": \"%s\",\n", forcing_index);
    fprintf(file, "  \"frequency\": \"%s\",\n", frequency);
    fprintf(file, "  \"grid_label\": \"g999\",\n");
    fprintf(file, "  \"host_collection\": \"CMIP7\",\n");
    fprintf(file, "  \"initialization_index\": \"i1\",\n");
    fprintf(file, "  \"institution_id\": \"MOHC\",\n");
    fprintf(file, "  \"license_id\": \"CC-BY-4.0\",\n");
    fprintf(file, "  \"nominal_resolution\": \"100 km\",\n");
    fprintf(file, "  \"outpath\": \"%s\",\n", output_dir);
    fprintf(file, "  \"physics_index\": \"p1\",\n");
    fprintf(file, "  \"realization_index\": \"%s\",\n", realization_index);
    fprintf(file, "  \"region\": \"glb\",\n");
    fprintf(file, "  \"source_id\": \"ACCESS-ESM1-6\"\n");
    fprintf(file, "}\n");
    fclose(file);
}

void cmip7_prepare_cmor(int argc, char **argv, const char *input_name,
                        const char *frequency, const char *realization_index,
                        const char *forcing_index, char *repo_root,
                        size_t repo_root_size, char *output_dir,
                        size_t output_dir_size, char *tables_path,
                        size_t tables_path_size)
{
    char input_path[CMIP7_PATH_MAX];
    char table_parent[CMIP7_PATH_MAX];
    int file_action = CMOR_REPLACE;
    int exit_control = CMOR_EXIT_ON_MAJOR;

    cmip7_get_example_args(argc, argv, repo_root, repo_root_size, output_dir,
                           output_dir_size);
    cmip7_ensure_directory(output_dir);
    cmip7_join_path(table_parent, sizeof(table_parent), repo_root,
                    "cmip7-cmor-tables");
    cmip7_join_path(tables_path, tables_path_size, table_parent, "tables");
    cmip7_join_path(input_path, sizeof(input_path), output_dir, input_name);
    cmip7_write_input_json(input_path, output_dir, frequency,
                           realization_index, forcing_index);

    cmip7_check_status("cmor_setup",
                       cmor_setup(tables_path, &file_action, NULL,
                                  &exit_control, NULL, NULL));
    cmip7_check_status("cmor_dataset_json", cmor_dataset_json(input_path));
}

int cmip7_load_table(const char *table_name)
{
    int table_id = -1;

    cmip7_check_status("cmor_load_table",
                       cmor_load_table((char *)table_name, &table_id));
    return table_id;
}

static void cmip7_compound_name(char *out, size_t out_size, const char *realm,
                                const char *table_entry,
                                const char *frequency, const char *region)
{
    char normalized[CMOR_MAX_STRING];
    size_t i;

    cmip7_copy_string(normalized, sizeof(normalized), table_entry);
    for (i = 0; normalized[i] != '\0'; ++i) {
        if (normalized[i] == '_') {
            normalized[i] = '.';
        }
    }
    snprintf(out, out_size, "%s.%s.%s.%s", realm, normalized, frequency,
             region);
}

static int cmip7_lookup_json_string(const char *path, const char *root_name,
                                    const char *key, char *value,
                                    size_t value_size)
{
    json_object *document = json_object_from_file(path);
    json_object *root = NULL;
    json_object *entry = NULL;

    value[0] = '\0';
    if (document == NULL) {
        fprintf(stderr, "Could not open CMIP7 metadata table %s\n", path);
        exit(1);
    }

    if (json_object_object_get_ex(document, root_name, &root) &&
        json_object_object_get_ex(root, key, &entry)) {
        cmip7_copy_string(value, value_size, json_object_get_string(entry));
        json_object_put(document);
        return 1;
    }

    json_object_put(document);
    return 0;
}

void cmip7_apply_variable_metadata(int var_id, const char *tables_path,
                                   const char *realm,
                                   const char *table_entry,
                                   const char *frequency,
                                   const char *region)
{
    char compound_name[CMOR_MAX_STRING];
    char path[CMIP7_PATH_MAX];
    char value[CMOR_MAX_STRING];

    cmip7_compound_name(compound_name, sizeof(compound_name), realm,
                        table_entry, frequency, region);

    cmip7_join_path(path, sizeof(path), tables_path,
                    "CMIP7_cell_measures.json");
    cmip7_lookup_json_string(path, "cell_measures", compound_name, value,
                             sizeof(value));
    cmip7_check_status("cmor_set_variable_attribute(cell_measures)",
                       cmor_set_variable_attribute(
                           var_id, "cell_measures", 'c', value));

    cmip7_join_path(path, sizeof(path), tables_path,
                    "CMIP7_long_name_overrides.json");
    if (cmip7_lookup_json_string(path, "long_name_overrides", compound_name,
                                 value, sizeof(value))) {
        cmip7_check_status("cmor_set_variable_attribute(long_name)",
                           cmor_set_variable_attribute(var_id, "long_name",
                                                       'c', value));
    }
}

void cmip7_close_variable(int var_id)
{
    char filename[CMOR_MAX_STRING];

    filename[0] = '\0';
    cmip7_check_status("cmor_close_variable",
                       cmor_close_variable(var_id, filename, NULL));
    printf("%s\n", filename);
    cmip7_check_status("cmor_close", cmor_close());
}

void cmip7_define_lon_lat_axes(const char *table_name, int *lon_id,
                               int *lat_id)
{
    double lat[CMIP7_NLAT] = {10.0, 20.0, 30.0};
    double lat_bnds[CMIP7_NLAT + 1] = {5.0, 15.0, 25.0, 35.0};
    double lon[CMIP7_NLON] = {0.0, 90.0, 180.0, 270.0};
    double lon_bnds[CMIP7_NLON + 1] = {-45.0, 45.0, 135.0, 225.0,
                                       315.0};

    cmip7_check_status("cmor_axis(latitude)",
                       cmor_axis(lat_id, "latitude", "degrees_north",
                                 CMIP7_NLAT, lat, 'd', lat_bnds, 1, NULL));
    cmip7_check_status("cmor_axis(longitude)",
                       cmor_axis(lon_id, "longitude", "degrees_east",
                                 CMIP7_NLON, lon, 'd', lon_bnds, 1, NULL));
    (void)table_name;
}

void cmip7_define_time_axis(const char *table_name, int *time_id)
{
    double time[CMIP7_NTIMES] = {15.5, 45.5};
    double time_bnds[CMIP7_NTIMES + 1] = {0.0, 31.0, 60.0};

    cmip7_check_status("cmor_axis(time)",
                       cmor_axis(time_id, "time",
                                 "days since 1979-01-01", CMIP7_NTIMES,
                                 time, 'd', time_bnds, 1, NULL));
    (void)table_name;
}

void cmip7_define_lon_lat_time_axes(const char *table_name, int *lon_id,
                                    int *lat_id, int *time_id)
{
    cmip7_define_lon_lat_axes(table_name, lon_id, lat_id);
    cmip7_define_time_axis(table_name, time_id);
}
