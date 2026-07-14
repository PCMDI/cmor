#include "cmip7_c_common.h"

#include <errno.h>
#include <json-c/json.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static void copy_string(char *out, size_t out_size, const char *value) {
  if (out_size > 0) {
    snprintf(out, out_size, "%s", value);
  }
}

static void join_path(char *out, size_t out_size, const char *left,
                      const char *right) {
  size_t left_len = strlen(left);

  if (left_len > 0 && left[left_len - 1] == '/') {
    snprintf(out, out_size, "%s%s", left, right);
  } else {
    snprintf(out, out_size, "%s/%s", left, right);
  }
}

static void ensure_directory(const char *path) {
  if (mkdir(path, 0775) != 0 && errno != EEXIST) {
    fprintf(stderr, "Could not create directory %s: %s\n", path,
            strerror(errno));
    exit(1);
  }
}

void cmip7_write_user_input_json(const char *output_dir, const char *input_name,
                                 const char *frequency,
                                 const char *realization_index,
                                 const char *forcing_index, char *input_path,
                                 size_t input_path_size) {
  FILE *file;

  ensure_directory(output_dir);
  join_path(input_path, input_path_size, output_dir, input_name);

  file = fopen(input_path, "w");
  if (file == NULL) {
    fprintf(stderr, "Could not write %s: %s\n", input_path, strerror(errno));
    exit(1);
  }

  fprintf(file, "{\n");
  fprintf(file, "  \"_AXIS_ENTRY_FILE\": \"CMIP7_coordinate.json\",\n");
  fprintf(file, "  \"_FORMULA_VAR_FILE\": \"CMIP7_formula_terms.json\",\n");
  fprintf(file, "  \"_cmip7_option\": 1,\n");
  fprintf(file, "  \"_controlled_vocabulary_file\": "
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

static void compound_name(char *out, size_t out_size, const char *realm,
                          const char *table_entry, const char *frequency,
                          const char *region) {
  char normalized[CMOR_MAX_STRING];
  size_t i;

  copy_string(normalized, sizeof(normalized), table_entry);
  for (i = 0; normalized[i] != '\0'; ++i) {
    if (normalized[i] == '_') {
      normalized[i] = '.';
    }
  }
  snprintf(out, out_size, "%s.%s.%s.%s", realm, normalized, frequency, region);
}

static int lookup_json_string(const char *path, const char *root_name,
                              const char *key, char *value, size_t value_size) {
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
    copy_string(value, value_size, json_object_get_string(entry));
    json_object_put(document);
    return 1;
  }

  json_object_put(document);
  return 0;
}

void cmip7_get_cell_measures(const char *tables_path, const char *realm,
                             const char *table_entry, const char *frequency,
                             const char *region, char *value,
                             size_t value_size) {
  char key[CMOR_MAX_STRING];
  char path[CMIP7_PATH_MAX];

  compound_name(key, sizeof(key), realm, table_entry, frequency, region);
  join_path(path, sizeof(path), tables_path, "CMIP7_cell_measures.json");
  lookup_json_string(path, "cell_measures", key, value, value_size);
}

int cmip7_get_long_name_override(const char *tables_path, const char *realm,
                                 const char *table_entry, const char *frequency,
                                 const char *region, char *value,
                                 size_t value_size) {
  char key[CMOR_MAX_STRING];
  char path[CMIP7_PATH_MAX];

  compound_name(key, sizeof(key), realm, table_entry, frequency, region);
  join_path(path, sizeof(path), tables_path, "CMIP7_long_name_overrides.json");
  return lookup_json_string(path, "long_name_overrides", key, value,
                            value_size);
}
