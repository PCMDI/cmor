/* -------------------------------------------------------------------- */
/*      this file contains function definitions for cmor                */
/*      cmor.c                                                          */
/*                                                                      */
/* -------------------------------------------------------------------- */
#ifndef CMOR_FUNC_H
#define CMOR_FUNC_H
#include <udunits2.h>
#include "cdmsint.h"
#include <stdio.h>
#include <json-c/json.h>

extern void cmor_md5( FILE * inputfile, unsigned char checksum[16] );

extern void cmor_set_terminate_signal_to_sigint();
extern void cmor_set_terminate_signal_to_sigterm();
extern void cmor_set_terminate_signal( int );
extern int cmor_get_terminate_signal();
extern void cmor_is_setup( void );
extern void cmor_add_traceback( char *name );
extern void cmor_pop_traceback( void );
extern int cmor_prep_units( char *uunits, char *cunits,
			    ut_unit ** user_units, ut_unit ** cmor_units,
			    cv_converter ** ut_cmor_converter );
extern int cmor_have_NetCDF4( void );
extern int cmor_have_NetCDF41min( void );
extern int cmor_have_NetCDF3( void );
extern int cmor_have_NetCDF363( void );
extern void cmor_handle_error( char error_msg[CMOR_MAX_STRING],
			       int level );
extern void cmor_handle_error_var( char error_msg[CMOR_MAX_STRING], int level,
                                   int var_id );
extern int cmor_setup( char *path, int *netcdf, int *verbosity, int *mode,
		       char *logfile, int *cmor_create_subdirectories);

extern int  cmor_outpath_exist(char *outpath);

extern int cmor_put_nc_num_attribute( int ncid, int nc_var_id, char *name,
				      char type, double value,
				      char *var_name );
extern int cmor_put_nc_char_attribute( int ncid, int nc_var_id, char *name,
				       char *value, char *var_name );
extern int cmor_set_cur_dataset_attribute( char *name, char *value,
					   int optional );
extern int cmor_set_cur_dataset_attribute_internal( char *name, char *value,
                                           int optional );

extern int cmor_get_cur_dataset_attribute( char *name, char *value );
extern int cmor_has_cur_dataset_attribute( char *name );
extern int cmor_get_table_attr( char *szToken, cmor_table_t * table, char *);

extern int cmor_dataset_json(char *rcfile);

extern int cmor_CreateFromTemplate(int vid, char *templateSTH, char *outpath,
                                   char *sep);

extern int cmor_addVersion(void);

extern int cmor_set_associated_file(int var_id, int nVarRefTblID );

extern int cmor_mkdir(const char *dir);
extern int cmor_create_filename(char *outname, int vid );
extern int cmor_IsFixed(int var_id);
extern int cmor_addRIPF(char *);
extern int cmor_set_refvar( int var_id, int *refvar, int ntimes_passed );
extern int strncpytrim( char *out, char *in, int max );
extern int cmor_convert_string_to_list( char *values, char type,
					void **target, int *nelts );
extern int cmor_define_zfactors_vars( int var_id, int ncid, int *nc_dim,
				      char *formula_terms, int *nzfactors,
				      int *zfactors, int *nc_zfactors,
				      int i, int dim_bnds );
extern int cmor_create_output_path( int var_id, char *outpath );
extern double cmor_convert_interval_to_seconds( double val, char *units );
extern int cmor_validateFilename(char *outname, char *suffix, int var_id);

extern int cmor_write( int var_id, void *data, char type, char *file_suffix,
		       int ntimes_passed, double *time_vals,
		       double *time_bounds, int *refvar );
extern int cmor_close_variable( int var_id, char *file_name,
				int *preserve );
extern int cmor_close( void );

extern int cmor_setDefaultGblAttr( int );
extern int cmor_writeGblAttr(int var_id, int ncid, int ncafid);
extern int cmor_setGblAttr( int );

extern void cmor_generate_uuid( void );
extern void cmor_define_dimensions(int var_id, int ncid,
                            int ncafid, double *time_bounds,
                            int *nc_dim,
                            int *nc_vars, int *nc_bnds_vars,
                            int *nc_vars_af,
                            size_t *nc_dim_chunking, int *dim_bnds,
                            int *zfactors, int *nc_zfactors,
                            int *nc_dim_af, int *nzfactors);

extern void cmor_create_var_attributes(int var_id, int ncid, int ncafid,
                                       int *nc_vars, int *nc_bnds_vars,
                                       int *nc_vars_af, int *nc_associated_vars,
                                       int *nc_singletons, int *nc_singletons_bnds,
                                       int *nc_zfactors, int *zfactors, int nzfactors,
                                       size_t *nc_dim_chunking, char *outname);

extern int cmor_grids_def(int var_id, int nGridID, int ncafid, int *nc_dim_af,
        int *nc_associated_vars);


extern void create_singleton_dimensions(int var_id, int ncid,
        int *nc_singletons, int *nc_singletons_bnds, int *dim_bnds);

extern int copyfile(const char *source, const char *dest);

/* ==================================================================== */
/*      Control Vocabulary                                              */
/* ==================================================================== */

extern int cmor_build_outname(int var_id, char *outname );
extern int cmor_CV_checkISOTime(char *szAttribute);
extern void cmor_CV_set_att(cmor_CV_def_t *CV,
                                char *key,
                                json_object *joValue);
extern int cmor_CV_checkFilename(cmor_CV_def_t *CV, int var_id,
        char *szInTimeCalendar,
        char *szInTimeUnits,
        char *infile);
extern int cmor_CV_checkParentExpID(cmor_CV_def_t *CV);
extern int cmor_CV_checkSubExpID(cmor_CV_def_t *CV);
extern int cmor_CV_checkExperiment( cmor_CV_def_t *CV);
extern int cmor_CV_checkSourceID(cmor_CV_def_t *CV);
extern int cmor_CV_checkSourceType(cmor_CV_def_t *CV, char *);
extern int get_CV_Error(void);
extern int cmor_attNameCmp(const void *v1, const void *v2);

extern int cmor_CV_checkGblAttributes( cmor_CV_def_t *CV );
extern void cmor_CV_free(cmor_CV_def_t *CV);
extern char *cmor_CV_get_value(cmor_CV_def_t *CV, char *key);
extern void cmor_CV_init( cmor_CV_def_t *CV, int table_id );
extern void cmor_CV_print(cmor_CV_def_t *CV);
extern void cmor_CV_printall( void );
extern cmor_CV_def_t *cmor_CV_search_child_key(cmor_CV_def_t *CV, char *key);
extern cmor_CV_def_t * cmor_CV_rootsearch(cmor_CV_def_t *CV, char *key);

extern int cmor_CV_checkFurtherInfoURL(int var_id);
extern int cmor_CV_checkGrids(cmor_CV_def_t *CV);
extern int cmor_CV_setInstitution( cmor_CV_def_t *CV);

extern int cmor_CV_set_entry(cmor_table_t* table, json_object *value);
extern int  cmor_CV_ValidateGblAttributes( char *name);
extern int cmor_CV_ValidateAttribute(cmor_CV_def_t *CV, char *szValue);



/* ==================================================================== */
/*      cmor_axis.c                                                     */
/* ==================================================================== */

extern void cmor_init_axis_def( cmor_axis_def_t * axis, int table_id );
extern int cmor_set_axis_def_att( cmor_axis_def_t * axis,
				  char att[CMOR_MAX_STRING],
				  char val[CMOR_MAX_STRING] );
extern void cmor_trim_string( char *in, char *out );
extern int cmor_calendar_c2i( char *calendar, cdCalenType * ical );
extern int cmor_convert_time_units( char *inunits, char *outunits,
				    char *loutunits );

extern void cmor_write_all_attributes(int ncid, int ncafid, int var_id);

extern int cmor_convert_time_values( void *values_in, char type,
				     int nvalues, double *values_out,
				     char *inunits, char *outunits,
				     char *calin, char *calout );
extern int cmor_set_axis_attribute( int id, char *attribute_name,
				    char type, void *value );
extern int cmor_get_axis_attribute( int id, char *attribute_name,
				    char type, void *value );
extern int cmor_has_axis_attribute( int id, char *attribute_name );
extern int cmor_check_values_inside_bounds( double *values, double *bounds,
					    int length, char *name );
extern int cmor_check_monotonic( double *values, int length, char *name,
				 int isbounds, int axis_id );
extern int cmor_treat_axis_values( int axis_id, double *values, int length,
				   int n_requested, char *units,
				   char *name, int isbounds );
extern int cmor_check_interval( int axis_id, char *interval,
				double *values, int nvalues,
				int isbounds );
extern int cmor_axis( int *axis_id, char *name, char *units, int length,
		      void *coord_vals, char type, void *cell_bounds,
		      int cell_bounds_ndim, char *interval );
/* ==================================================================== */
/*       cmor_variable.c                                                */
/* ==================================================================== */

extern void cmor_init_var_def( cmor_var_def_t * var, int table_id );
extern int cmor_is_required_variable_attribute( cmor_var_def_t var,
						char *attribute_name );
extern int cmor_has_required_variable_attributes( int var_id );
extern int cmor_set_variable_attribute( int id, char *attribute_name,
					char type, void *value );
extern int cmor_set_variable_attribute_internal( int id,
                                                 char *attribute_name,
                                                 char type, void *value );

extern int cmor_get_variable_attribute( int id, char *attribute_name,
					void *value );
extern int cmor_has_variable_attribute( int id, char *attribute_name );
extern int cmor_get_variable_attribute_names( int id, int *nattributes,
					      char
					      attributes_names[]
					      [CMOR_MAX_STRING] );
extern int cmor_get_variable_attribute_type( int id, char *attribute_name,
					     char *type );
extern int cmor_zfactor( int *zvar_id, int axis_id, char *name,
			 char *units, int ndims, int axes_ids[], char type,
			 void *values, void *bounds );
extern int cmor_update_history( int var_id, char *add );
extern int cmor_variable( int *var_id, char *name, char *units, int ndims,
			  int axes_ids[], char type, void *missing,
			  double *tolerance, char *positive,
			  char *original_name, char *history,
			  char *comment );
extern int cmor_set_deflate( int var_id, int shuffle,
                             int deflate, int deflate_level );
extern int cmor_set_zstandard( int var_id, int zstandard_level );
extern int cmor_set_quantize( int var_id, int quantize_mode, 
                              int quantize_nsd );
extern int cmor_set_chunking( int var_id, int nTableID,
							    size_t nc_dim_chunking[]);

extern int cmor_set_var_def_att( cmor_var_def_t * var,
				 char att[CMOR_MAX_STRING],
				 char val[CMOR_MAX_STRING] );
extern int cmor_get_variable_time_length( int *var_id, int *length );
extern int cmor_get_original_shape( int *var_id, int *shape_array,
				    int *rank, int blank_time );
extern int cmor_write_var_to_file( int ncid, cmor_var_t * avar, void *data,
				   char itype, int ntimes_passed,
				   double *time_vals,
				   double *time_bounds );
/* ==================================================================== */
/*      cmor_grid.c                                                     */
/* ==================================================================== */

extern void cmor_set_mapping_attribute( cmor_mappings_t * mapping,
					char att[CMOR_MAX_STRING],
					char val[CMOR_MAX_STRING] );
extern void cmor_init_grid_mapping( cmor_mappings_t * mapping, char *id );
extern int cmor_copy_data( double **dest1, void *data, char type,
			   int nelts );
extern int cmor_has_grid_attribute( int gid, char *name );
extern int cmor_get_grid_attribute( int gid, char *name, double *value );
extern void cmor_convert_value( char *units, char *ctmp, double *tmp );
extern int cmor_set_grid_attribute( int gid, char *name, double *value,
				    char *units );
extern int cmor_attribute_in_list( char *name, int n,
				   char ( *atts )[CMOR_MAX_STRING] );
extern int cmor_grid_valid_mapping_attribute_names( char *name, int *natt,
						    char ( *att )
						    [CMOR_MAX_STRING],
						    int *ndims,
						    char ( *dims )
						    [CMOR_MAX_STRING] );
extern int cmor_set_grid_mapping( int gid, char *name, int nparam,
				  char *attributes_names, int lparams,
				  double attributes_values[CMOR_MAX_GRID_ATTRIBUTES],
				  char *units,
				  int lnunits );
extern int cmor_grid( int *grid_id, int ndims, int *axes_ids, char type,
		      void *lat, void *lon, int nvertices, void *blat,
		      void *blon );
extern void cmor_init_table( cmor_table_t * table, int id );
extern int cmor_set_dataset_att( cmor_table_t * table,
				 char att[CMOR_MAX_STRING],
				 char val[CMOR_MAX_STRING] );
extern int cmor_set_experiment( cmor_table_t * table,
                                char *att,
                                char *var );

extern int cmor_set_axis_entry(cmor_table_t* table,
                               char *axis_entry,
                               json_object *json);

extern int cmor_set_variable_entry(cmor_table_t* table,
                            char *variable_entry,
                            json_object *json);

extern int cmor_set_formula_entry(cmor_table_t* table,
                            char *variable_entry,
                            json_object *json);

extern int cmor_set_table( int table );
extern int cmor_load_table( char table[CMOR_MAX_STRING], int *table_id );
extern int cmor_load_table_internal( char table[CMOR_MAX_STRING],
                                     int *table_id );
extern int cmor_search_table( char szTable[CMOR_MAX_STRING],
								int *table_id);
extern int cmor_validate_json(json_object *json);

extern json_object *cmor_open_inpathFile( char *szFilename);

extern int cmor_load_ressources( char *szFilename );

extern int cmor_time_varying_grid_coordinate( int *coord_grid_id,
					      int grid_id, char *name,
					      char *units, char type,
					      void *missing,
					      int *coordinate_type );
extern void cmor_cat_unique_string( char *dest, char *src );
extern int cmor_stringinstring( char *dest, char *src );
extern void cmor_checkMissing(int varid, int var_id, char type);
extern char *cmor_getFinalFilename( void );

#endif
