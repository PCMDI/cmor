#ifndef CMIP7_C_COMMON_H
#define CMIP7_C_COMMON_H

#include <stddef.h>

#include "cmor.h"

#define CMIP7_NLON 4
#define CMIP7_NLAT 3
#define CMIP7_NTIMES 2
#define CMIP7_MISSING_VALUE 1.0e20f
#define CMIP7_PATH_MAX 4096

void cmip7_get_example_args(int argc, char **argv, char *repo_root,
                            size_t repo_root_size, char *output_dir,
                            size_t output_dir_size);
void cmip7_prepare_cmor(int argc, char **argv, const char *input_name,
                        const char *frequency, const char *realization_index,
                        const char *forcing_index, char *repo_root,
                        size_t repo_root_size, char *output_dir,
                        size_t output_dir_size, char *tables_path,
                        size_t tables_path_size);
int cmip7_load_table(const char *table_name);
void cmip7_apply_variable_metadata(int var_id, const char *tables_path,
                                   const char *realm,
                                   const char *table_entry,
                                   const char *frequency,
                                   const char *region);
void cmip7_close_variable(int var_id);
void cmip7_check_status(const char *call_name, int ierr);
void cmip7_join_path(char *out, size_t out_size, const char *left,
                     const char *right);

void cmip7_define_lon_lat_time_axes(const char *table_name, int *lon_id,
                                    int *lat_id, int *time_id);
void cmip7_define_lon_lat_axes(const char *table_name, int *lon_id,
                               int *lat_id);
void cmip7_define_time_axis(const char *table_name, int *time_id);

#endif
