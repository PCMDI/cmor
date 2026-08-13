#ifndef CMIP7_C_COMMON_H
#define CMIP7_C_COMMON_H

#include "cmor.h"
#include <stddef.h>

#define CMIP7_NLON 4
#define CMIP7_NLAT 3
#define CMIP7_NTIMES 2
#define CMIP7_MISSING_VALUE 1.0e20f
#define CMIP7_PATH_MAX 4096

void cmip7_write_user_input_json(const char *output_dir, const char *input_name,
                                 const char *frequency,
                                 const char *realization_index,
                                 const char *forcing_index, char *input_path,
                                 size_t input_path_size);
void cmip7_get_cell_measures(const char *tables_path, const char *realm,
                             const char *table_entry, const char *frequency,
                             const char *region, char *value,
                             size_t value_size);
int cmip7_get_long_name_override(const char *tables_path, const char *realm,
                                 const char *table_entry, const char *frequency,
                                 const char *region, char *value,
                                 size_t value_size);

#endif
