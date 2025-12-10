#ifndef CMOR_H
#define CMOR_H

#define CMOR_VERSION_MAJOR 3
#define CMOR_VERSION_MINOR 13
#define CMOR_VERSION_PATCH 0

#define CMOR_CF_VERSION_MAJOR 1
#define CMOR_CF_VERSION_MINOR 7

#define CMOR_MAX_STRING 1024
#define CMOR_DEF_ATT_STR_LEN 256
#define CMOR_MAX_ELEMENTS 500
#define CMOR_MAX_FORMULA 40
#define CMOR_MAX_AXES CMOR_MAX_ELEMENTS*3
#define CMOR_MAX_VARIABLES CMOR_MAX_ELEMENTS
#define CMOR_MAX_GRIDS 100
#define CMOR_MAX_DIMENSIONS 7
#define CMOR_MAX_ATTRIBUTES 100
#define CMOR_MAX_ERRORS 10
#define CMOR_MAX_TABLES 30
#define CMOR_MAX_GRID_ATTRIBUTES 25
#define CMOR_MAX_JSON_ARRAY 50
#define CMOR_MAX_JSON_OBJECT 250

#define CMOR_QUIET 0

#define CMOR_EXIT_ON_MAJOR 0
#define CMOR_EXIT 1
#define CMOR_EXIT_ON_WARNING 2

#define CMOR_WARNING 20
#define CMOR_NORMAL 21
#define CMOR_CRITICAL 22
#define CMOR_NOT_SETUP 23

#define CMOR_N_VALID_CALS 8

#define CMOR_PRESERVE_4 10
#define CMOR_APPEND_4   11
#define CMOR_REPLACE_4  12

#define CMOR_PRESERVE_3 13
#define CMOR_APPEND_3   14
#define CMOR_REPLACE_3  15

#define CMOR_PRESERVE CMOR_PRESERVE_4
#define CMOR_APPEND CMOR_APPEND_4
#define CMOR_REPLACE CMOR_REPLACE_4

#define CMOR_APPROX_INTERVAL_DEFAULT          0.
#define CMOR_APPROX_INTERVAL_WARNING_DEFAULT  .1
#define CMOR_APPROX_INTERVAL_ERROR_DEFAULT    .2

#define CMOR_INPUTFILENAME       GLOBAL_INTERNAL"dataset_json"
#define CV_INPUTFILENAME         GLOBAL_INTERNAL"CV"
#define CV_CHECK_ERROR           GLOBAL_INTERNAL"CV_ERROR"
/* -------------------------------------------------------------------- */
/*      Define AXIS attribue strings                                    */
/* -------------------------------------------------------------------- */

#define CMOR_AXIS_ENTRY_FILE     GLOBAL_INTERNAL"AXIS_ENTRY_FILE"
#define AXIS_ATT_REQUIRED        "required"
#define AXIS_ATT_ID              "id"
#define AXIS_ATT_CLIMATOLOGY     "climatology"
#define AXIS_ATT_OUTNAME         "out_name"
#define AXIS_ATT_STANDARDNAME    "standard_name"
#define AXIS_ATT_LONGNAME        "long_name"
#define AXIS_ATT_CONVERTTO       "convert_to"
#define AXIS_ATT_FORMULA         "formula"
#define AXIS_ATT_ZFACTORS        "z_factors"
#define AXIS_ATT_ZBOUNDSFACTORS  "z_bounds_factors"
#define AXIS_ATT_UNITS           "units"
#define AXIS_ATT_STOREDDIRECTION "stored_direction"
#define AXIS_ATT_POSITIVE        "positive"
#define AXIS_ATT_GEN_LEVEL_NAME  "generic_level_name"
#define AXIS_ATT_FORECAST        "forecast"
#define AXIS_ATT_AXIS            "axis"
#define AXIS_ATT_INDEXONLY       "index_only"
#define AXIS_ATT_MUSTBOUNDS      "must_have_bounds"
#define AXIS_ATT_MUSTCALLGRID    "must_call_cmor_grid"
#define AXIS_ATT_TYPE            "type"
#define AXIS_ATT_VALIDMIN        "valid_min"
#define AXIS_ATT_VALIDMAX        "valid_max"
#define AXIS_ATT_TOLERANCE       "tolerance"
#define AXIS_ATT_TOLONREQUESTS   "tol_on_requests"
#define AXIS_ATT_VALUE           "value"
#define AXIS_ATT_BOUNDVALUES     "bounds_values"
#define AXIS_ATT_COORDSATTRIB    "coords_attrib"
#define AXIS_ATT_BOUNDSREQUESTED "bounds_requested"
#define AXIS_ATT_REQUESTEDBOUNDS "requested_bounds"
#define AXIS_ATT_REQUESTED       "requested"
#define AXIS_FORECAST_TIME       "reftime"
#define AXIS_FORECAST_LEADTIME   "leadtime"

#define TABLE_CONTROL_FILENAME    "CMIP6_CV.json"
#define TABLE_FOUND               -1
#define TABLE_ERROR                1
#define TABLE_SUCCESS              0
#define TABLE_NOTFOUND            -2

#define FILE_ERROR                 1
#define FILE_PATH_TEMPLATE         "output_path_template"
#define FILE_NAME_TEMPLATE         "output_file_template"
#define FILE_OUTPUTPATH            "outpath"

#define FORMULA_VAR_FILENAME          "CMIP6_formula_terms.json"
#define AXIS_ENTRY_FILENAME           "CMIP6_coordinate.json"
#define CMOR_FORMULA_VAR_FILE         GLOBAL_INTERNAL"FORMULA_VAR_FILE"
#define VARIABLE_ATT_UNITS            "units"
#define VARIABLE_ATT_MISSINGVALUES    "missing_value"
#define VARIABLE_ATT_FILLVAL          "_FillValue"
#define VARIABLE_ATT_STANDARDNAME     "standard_name"
#define VARIABLE_ATT_LONGNAME         "long_name"
#define VARIABLE_ATT_FLAGVALUES       "flag_values"
#define VARIABLE_ATT_FLAGMEANING      "flag_meaning"
#define VARIABLE_ATT_COMMENT          "comment"
#define VARIABLE_ATT_HISTORY          "history"
#define VARIABLE_ATT_ORIGINALNAME     "original_name"
#define VARIABLE_ATT_ORIGINALUNITS    "original_units"
#define VARIABLE_ATT_POSITIVE         "positive"
#define VARIABLE_ATT_CELLMETHODS      "cell_methods"
#define VARIABLE_ATT_COORDINATES      "coordinates"
#define VARIABLE_ATT_REQUIRED         "required"
#define VARIABLE_ATT_DIMENSIONS       "dimensions"
#define VARIABLE_ATT_ID               "id"
#define VARIABLE_ATT_TYPE             "type"
#define VARIABLE_ATT_UNITS            "units"
#define VARIABLE_ATT_EXTCELLMEASURES  "ext_cell_measures"
#define VARIABLE_ATT_CELLMEASURES     "cell_measures"
#define VARIALBE_ATT_GRIDMAPPING      "grid_mapping"
#define VARIALBE_ATT_QUANTIZATION     "quantization"
#define VARIALBE_ATT_QUANTIZATION_NSB "quantization_nsb"
#define VARIALBE_ATT_QUANTIZATION_NSD "quantization_nsd"

#define VARIABLE_ATT_VALIDMIN         "valid_min"
#define VARIABLE_ATT_VALIDMAX         "valid_max"
#define VARIABLE_ATT_MINMEANABS       "ok_min_mean_abs"
#define VARIABLE_ATT_MAXMEANABS       "ok_max_mean_abs"
#define VARIABLE_ATT_CHUNKING         "chunk_dimensions"
#define VARIABLE_ATT_SHUFFLE          "shuffle"
#define VARIABLE_ATT_DEFLATE          "deflate"
#define VARIABLE_ATT_DEFLATELEVEL     "deflate_level"
#define VARIABLE_ATT_ZSTANDARDLEVEL   "zstandard_level"
#define VARIABLE_ATT_MODELINGREALM    "modeling_realm"
#define VARIALBE_ATT_FREQUENCY        "frequency"
#define VARIABLE_ATT_FLAGVALUES       "flag_values"
#define VARIABLE_ATT_FLAGMEANINGS     "flag_meanings"
#define VARIABLE_ATT_OUTNAME          "out_name"
#define VARIABLE_ATT_BRANDDESCRIPTION "brand_description"
#define VARIABLE_ATT_BRANDEDVARIABLE  "branded_variable"
#define VARIABLE_ATT_BRANDINGSUFFIX   "branding_suffix"
#define VARIABLE_ATT_TEMPORALLABEL    "temporal_label"
#define VARIABLE_ATT_VERTICALLABEL    "vertical_label"
#define VARIABLE_ATT_HORIZONTALLABEL  "horizontal_label"
#define VARIABLE_ATT_AREALABEL        "area_label"
#define VARIABLE_ATT_VARIABLE_TITLE   "variable_title"
#define COMMENT_VARIABLE_ZFACTOR      "use formula table"
#define GLOBAL_SEPARATORS             "><"
#define GLOBAL_OPENOPTIONAL           "["
#define GLOBAL_CLOSEOPTIONAL          "]"
#define GLOBAL_INTERNAL               "_"
#define GLOBAL_ATT_FORCING            "forcing"
#define GLOBAL_ATT_PRODUCT            "product"
#define GLOBAL_ATT_EXPERIMENTID       "experiment_id"
#define GLOBAL_ATT_EXPERIMENT         "experiment"
#define GLOBAL_ATT_MODELID            "model_id"
#define GLOBAL_ATT_REALIZATION        "realization_index"
#define GLOBAL_ATT_INITIA_IDX         "initialization_index"
#define GLOBAL_ATT_PHYSICS_IDX        "physics_index"
#define GLOBAL_ATT_FORCING_IDX        "forcing_index"
#define GLOBAL_ATT_CMORVERSION        "cmor_version"
#define GLOBAL_ATT_ACTIVITY_ID        "activity_id"
#define GLOBAL_ATT_VAL_NODRIVER       "no-driver"
#define GLOBAL_ATT_VARIABLE_ID        "variable_id"
#define GLOBAL_ATT_BRANDEDVARIABLE    "branded_variable"
#define GLOBAL_ATT_BRANDINGSUFFIX     "branding_suffix"
#define GLOBAL_ATT_TEMPORALLABEL      "temporal_label"
#define GLOBAL_ATT_VERTICALLABEL      "vertical_label"
#define GLOBAL_ATT_HORIZONTALLABEL    "horizontal_label"
#define GLOBAL_ATT_AREALABEL          "area_label"
#define GLOBAL_ATT_SOURCE_ID          "source_id"
#define GLOBAL_ATT_SOURCE             "source"
#define GLOBAL_ATT_SUB_EXPT_ID        "sub_experiment_id"
#define GLOBAL_ATT_SUB_EXPT           "sub_experiment"
#define GLOBAL_ATT_SOURCE_TYPE        "source_type"
#define GLOBAL_ATT_CONVENTIONS        "Conventions"
#define GLOBAL_ATT_CREATION_DATE      "creation_date"
#define GLOBAL_ATT_HISTORY            "history"
#define GLOBAL_ATT_HISTORYTMPL         GLOBAL_INTERNAL"history_template"
#define GLOBAL_ATT_TABLE_ID           "table_id"
#define GLOBAL_ATT_TABLE_INFO         "table_info"
#define GLOBAL_ATT_EXPERIMENT         "experiment"
#define GLOBAL_ATT_TITLE              "title"
#define GLOBAL_ATT_PARENT_EXPT_ID     "parent_experiment_id"
#define GLOBAL_ATT_BRANCH_TIME        "branch_time"
#define GLOBAL_ATT_REALM              "realm"
#define GLOBAL_ATT_TRACKING_ID        "tracking_id"
#define GLOBAL_ATT_VARIANT_LABEL      "variant_label"
#define GLOBAL_ATT_MEMBER_ID          GLOBAL_INTERNAL"member_id"
#define GLOBAL_ATT_DATASPECSVERSION   "data_specs_version"
#define GLOBAL_ATT_FREQUENCY          "frequency"
#define GLOBAL_ATT_LICENSE            "license"
#define GLOBAL_ATT_LICENSE_ID         "license_id"
#define GLOBAL_ATT_LICENSE_TYPE       "license_type"
#define GLOBAL_ATT_LICENSE_URL        "license_url"
#define GLOBAL_ATT_TRACKING_PREFIX    "tracking_prefix"
#define GLOBAL_ATT_CALENDAR           "calendar"
#define GLOBAL_ATT_INSTITUTION_ID     "institution_id"
#define GLOBAL_ATT_INSTITUTION        "institution"
#define GLOBAL_ATT_FURTHERINFOURL     "further_info_url"
#define GLOBAL_ATT_FURTHERINFOURLTMPL GLOBAL_INTERNAL"further_info_url_tmpl"
#define GLOBAL_ATT_GRID_LABEL         "grid_label"
#define GLOBAL_ATT_GRID_RESOLUTION    "nominal_resolution"
#define GLOBAL_ATT_TITLE_MSG          "%s output prepared for %s"
#define GLOBAL_ATT_EXTERNAL_VAR       "external_variables"
#define GLOBAL_ATT_MIP_ERA            "mip_era"
#define GLOBAL_CV_FILENAME            GLOBAL_INTERNAL"controlled_vocabulary_file"
#define GLOBAL_IS_CMIP6               GLOBAL_INTERNAL"cmip6_option"
#define GLOBAL_IS_CMIP7               GLOBAL_INTERNAL"cmip7_option"
#define GLOBAL_USE_STRING_INDEXES     GLOBAL_INTERNAL"use_strings_for_indexes"

#define NO_PARENT                     "no parent"
#define NONE                          "none"
#define BRANCH_METHOD                 "branch_method"
#define BRANCH_TIME_IN_CHILD          "branch_time_in_child"
#define BRANCH_TIME_IN_PARENT         "branch_time_in_parent"
#define PARENT_ACTIVITY_ID            "parent_activity_id"
#define PARENT_EXPERIMENT_ID          "parent_experiment_id"
#define PARENT_MIP_ERA                "parent_mip_era"
#define PARENT_SOURCE_ID              "parent_source_id"
#define PARENT_TIME_UNITS             "parent_time_units"
#define PARENT_VARIANT_LABEL          "parent_variant_label"

#define JSON_KEY_HEADER               "Header"
#define JSON_KEY_EXPERIMENT           "experiments"
#define JSON_KEY_AXIS_ENTRY           "axis_entry"
#define JSON_KEY_VARIABLE_ENTRY       "variable_entry"
#define JSON_KEY_FORMULA_ENTRY        "formula_entry"
#define JSON_KEY_MAPPING_ENTRY        "mapping_entry"
#define JSON_KEY_CV_ENTRY             "CV"

#define CV_KEY_REQUIRED_GBL_ATTRS     "required_global_attributes"
#define CV_KEY_FREQUENCY              "frequency"
#define CV_KEY_INSTITUTION_ID         "institution_id"
#define CV_KEY_EXPERIMENT_ID          "experiment_id"
#define CV_KEY_SOURCE_IDS             "source_id"
#define CV_KEY_GRID_LABELS            "grid_label"
#define CV_KEY_GRID_RESOLUTION        "nominal_resolution"
#define CV_KEY_GRIDLABEL_GR           "gr"
#define CV_KEY_SOURCE_LABEL           "source"
#define CV_KEY_SUB_EXPERIMENT_ID      "sub_experiment_id"
#define CV_KEY_BRANDING_TEMPLATE      "branding_suffix"
#define CV_KEY_APRX_INTRVL            "approx_interval"
#define CV_KEY_APRX_INTRVL_ERR        "approx_interval_error"
#define CV_KEY_APRX_INTRVL_WRN        "approx_interval_warning"
#define CV_KEY_DATASPECSVERSION       "data_specs_version"
#define CV_KEY_MIP_ERA                "mip_era"
#define CV_KEY_LICENSE                "license"
#define CV_KEY_LICENSE_ID             "license_id"
#define CV_KEY_LICENSE_TEMPLATE       "license_template"
#define CV_KEY_LICENSE_TYPE           "license_type"
#define CV_KEY_LICENSE_URL            "license_url"
#define CV_KEY_VERSION_METADATA       "version_metadata"
#define CV_KEY_DRS                    "DRS"
#define CV_KEY_DIR_PATH_TEMPLATE      "directory_path_template"
#define CV_KEY_FILENAME_TEMPLATE      "filename_template"

#define CV_EXP_ATTR_ADDSOURCETYPE     "additional_allowed_model_components"
#define CV_EXP_ATTR_REQSOURCETYPE     "required_model_components"
#define CV_EXP_ATTR_DESCRIPTION       "description"
#define GLOBAL_INT_ATT_PARENT_EXPT    GLOBAL_INTERNAL"parent_experiment"
#define GLOBAL_ATT_VERSION            GLOBAL_INTERNAL"version"

#define TABLE_HEADER_VERSION          "cmor_version"
#define TABLE_HEADER_GENERIC_LEVS     "generic_levels"
#define TABLE_HEADER_CF_VERSION       "cf_version"
#define TABLE_HEADER_CONVENTIONS      "Conventions"
#define TABLE_HEADER_MIP_ERA          "mip_era"
#define TABLE_HEADER_REALM            "realm"
#define TABLE_HEADER_TABLE_DATE       "table_date"
#define TABLE_HEADER_FORCINGS         "forcings"
#define TABLE_HEADER_TABLE_ID         "table_id"
#define TABLE_HEADER_BASEURL          "baseURL"
#define TABLE_HEADER_PRODUCT          "product"
#define TABLE_EXPIDS                  "expt_id_ok"
#define TABLE_HEADER_APRX_INTRVL      "approx_interval"
#define TABLE_HEADER_APRX_INTRVL_ERR  "approx_interval_error"
#define TABLE_HEADER_APRX_INTRVL_WRN  "approx_interval_warning"
#define TABLE_HEADER_MISSING_VALUE    "missing_value"
#define TABLE_HEADER_INT_MISSING_VALUE    "int_missing_value"
#define TABLE_HEADER_MAGIC_NUMBER     "magic_number"
#define TABLE_HEADER_DATASPECSVERSION "data_specs_version"
#define TABLE_HEADER_VALIDMIN         "valid_min"
#define TABLE_HEADER_VALIDMAX         "valid_max"
#define TABLE_HEADER_MINMEANABS       "ok_min_mean_abs"
#define TABLE_HEADER_MAXMEANABS       "ok_max_mean_abs"
#define TABLE_HEADER_POSITIVE         "positive"
#define TABLE_HEADER_CHECKSUM         "checksum"
#define TABLE_HEADER_TYPE             "type"
#define OUTPUT_TEMPLATE_RIPF          "run_variant"

#define DIMENSION_LATITUDE            "latitude"
#define DIMENSION_LONGITUDE           "longitude"
#define DIMENSION_ALEVEL              "alevel"
#define DIMENSION_ALEVEL_HALF         "alevhalf"
#define DIMENSION_ZLEVEL              "zlevel"
#define DIMENSION_OLEVEL              "olevel"
#define DIMENSION_OLEVEL_HALF         "olevhalf"

#define AREA                          "area"
#define VOLUME                        "volume"
#define CMIP6                         "CMIP6"
#define CMIP7                         "CMIP7"
#define CMOR_DEFAULT_PATH_TEMPLATE    "<mip_era><activity_id><institution_id><source_id><experiment_id><member_id><table><variable_id><grid_label><version>"
#define CMOR_DEFAULT_FILE_TEMPLATE    "<variable_id><table><source_id><experiment_id><member_id><grid_label>"
#define CMOR_DEFAULT_FURTHERURL_TEMPLATE "https://furtherinfo.es-doc.org/<mip_era><institution_id><source_id><experiment_id><sub_experiment_id><variant_label>"
#define CMOR_DEFAULT_HISTORY_TEMPLATE "%s ; CMOR rewrote data to be consistent with <mip_era>, <Conventions> and CF standards."
//#define EXTERNAL_VARIABLE_REGEX       "([[:alpha:]]+):[[:blank:]]*([[:alpha:]]+)[[:blank:]]*([[:alpha:]]+:[[:blank:]]*([[:alpha:]]+))*"
#define EXTERNAL_VARIABLE_REGEX       "[[:alpha:]]+:[[:blank:]]*([[:alpha:]]+)([[:blank:]]*[[:alpha:]]+:[[:blank:]]*([[:alpha:]]+))*"

#define QUANTIZATION_INFO              "quantization_info"
#define QUANTIZATION_ALGORITHM         "algorithm"
#define QUANTIZATION_IMPLEMENTATION    "implementation"
#define QUANTIZATION_NOQUANTIZATION    "no_quantization"
#define QUANTIZATION_BITGROOM          "bitgroom"
#define QUANTIZATION_BITROUND          "bitround"
#define QUANTIZATION_GRANULARBITROUND  "granular_bitround"

extern int CMOR_TERMINATE_SIGNAL;
extern int USE_NETCDF_4;
extern int CMOR_MODE;
extern int CMOR_TABLE;
extern int CMOR_VERBOSITY;
extern int CMOR_NETCDF_MODE;
extern int CV_ERROR;

extern int clfeanup_varid;

extern int cmor_naxes;
extern int cmor_nvars;
extern int cmor_ntables;
extern int cmor_ngrids;

extern int cmor_nerrors;
extern int cmor_nwarnings;

extern char cmor_input_path[CMOR_MAX_STRING];

extern char cmor_traceback_info[CMOR_MAX_STRING];

typedef struct cmor_grid_ {
    int id;
    char name[CMOR_MAX_STRING];
    char mapping[CMOR_MAX_STRING];
    int nattributes;
    char attributes_names[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    double attributes_values[CMOR_MAX_GRID_ATTRIBUTES];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    int original_axes_ids[CMOR_MAX_DIMENSIONS];
    int ndims;
    int istimevarying;
    int nvertices;
    double *lons;
    double *lats;
    double *blons;
    double *blats;
/* -------------------------------------------------------------------- */
/*      for lon/lat/blon/blat/area/volumes                              */
/* -------------------------------------------------------------------- */
    int associated_variables[6];
    int ntextattributes;
    char text_attributes_names[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    char *text_attributes_values[CMOR_MAX_GRID_ATTRIBUTES];
} cmor_grid_t;

extern cmor_grid_t cmor_grids[CMOR_MAX_GRIDS];

typedef struct cmor_axis_def_ {
    int table_id;
    int climatology;
    char id[CMOR_MAX_STRING];
    char standard_name[CMOR_MAX_STRING];
    char units[CMOR_MAX_STRING];
    char axis;
    char positive;
    char long_name[CMOR_MAX_STRING];
    char out_name[CMOR_MAX_STRING];
    char type;
    char stored_direction;
    double valid_min;
    double valid_max;
    int n_requested;
    double *requested;
    char *crequested;
    char cname[CMOR_MAX_STRING];
    int n_requested_bounds;
    double *requested_bounds;
    double tolerance;
    double value;
    char cvalue[CMOR_MAX_STRING];
    double bounds_value[2];
    char required[CMOR_MAX_STRING];
    char formula[CMOR_MAX_STRING];
    char convert_to[CMOR_MAX_STRING];
    char z_factors[CMOR_MAX_STRING];
    char z_bounds_factors[CMOR_MAX_STRING];
    char index_only;
    int must_have_bounds;
    int must_call_cmor_grid;
    char generic_level_name[CMOR_MAX_STRING];
    char forecast[CMOR_MAX_STRING];
} cmor_axis_def_t;

enum CV_type {
    CV_undef,
    CV_integer,
    CV_double,
    CV_string,
    CV_stringarray,
    CV_object,
};


typedef struct cmor_CV_def_ {
    int     table_id;
    char    key[CMOR_MAX_STRING];
    int     type;
    int     nValue;
    double  dValue;
    char    szValue[CMOR_MAX_STRING];
    char    **aszValue;
    int     anElements;
    int     nbObjects;
    struct cmor_CV_def_ *oValue;
} cmor_CV_def_t;

typedef struct cmor_axis_ {
    int ref_table_id;
    int ref_axis_id;
    int isgridaxis;
    char axis;
    char iunits[CMOR_MAX_STRING];
    char id[CMOR_MAX_STRING];
    int length;
    double *values;
    int *wrapping;
    double *bounds;
    char **cvalues;
    int revert;
    int offset;
    char type;
    char attributes_values_char[CMOR_MAX_ATTRIBUTES][CMOR_MAX_STRING];
    double attributes_values_num[CMOR_MAX_ATTRIBUTES];
    char attributes_type[CMOR_MAX_ATTRIBUTES];	/*stores attributes type */
    char attributes[CMOR_MAX_ATTRIBUTES][CMOR_MAX_STRING];	/*stores attributes names */
    int nattributes;		/* number of character type attributes */
    int hybrid_in;
    int hybrid_out;
    int store_in_netcdf;
    double approx_interval;
    double approx_interval_warning;
    double approx_interval_error;
} cmor_axis_t;
extern cmor_axis_t cmor_axes[CMOR_MAX_AXES];

typedef struct cmor_variable_def_ {
    int table_id;
    char id[CMOR_MAX_STRING];
    char brand_description[CMOR_MAX_STRING];
    char variable_title[CMOR_MAX_STRING];
    char standard_name[CMOR_MAX_STRING];
    char units[CMOR_MAX_STRING];
    char cell_methods[CMOR_MAX_STRING];
    char cell_measures[CMOR_MAX_STRING];
    char positive;
    char flag_values[CMOR_MAX_STRING];
    char flag_meanings[CMOR_MAX_STRING];
    char long_name[CMOR_MAX_STRING];
    char comment[CMOR_MAX_STRING];
    int ndims;
    int dimensions[CMOR_MAX_DIMENSIONS];
    char dimensions_str[CMOR_MAX_STRING];
    char type;
    float valid_min;
    float valid_max;
    float ok_min_mean_abs;
    float ok_max_mean_abs;
    char chunking_dimensions[CMOR_MAX_STRING];
    int shuffle;
    int deflate;
    int deflate_level;
    int zstandard_level;
    char required[CMOR_MAX_STRING];
    char realm[CMOR_MAX_STRING];
    char frequency[CMOR_MAX_STRING];
    char out_name[CMOR_MAX_STRING];
    char generic_level_name[CMOR_MAX_STRING];
    char branded_variable[CMOR_MAX_STRING];
    char branding_suffix[CMOR_MAX_STRING];
    char temporal_label[CMOR_MAX_STRING];
    char vertical_label[CMOR_MAX_STRING];
    char horizontal_label[CMOR_MAX_STRING];
    char area_label[CMOR_MAX_STRING];
} cmor_var_def_t;

typedef struct cmor_var_ {
    int self;
    int grid_id;
    int sign;
    int zfactor;
    int ref_table_id;
    int ref_var_id;
    int initialized;
    int error;
    int closed;
    int nc_var_id;
    int nc_zfactors[CMOR_MAX_VARIABLES];
    int nzfactor;
    int ntimes_written;
    double last_time_written;
    double last_time_bounds_written[2];
    int ntimes_written_coords[10];
    int associated_ids[10];
    int ntimes_written_associated[10];
    int time_nc_id;
    int time_bnds_nc_id;
    char id[CMOR_MAX_STRING];
    int ndims;
    int singleton_ids[CMOR_MAX_DIMENSIONS];
    int axes_ids[CMOR_MAX_DIMENSIONS];
    int original_order[CMOR_MAX_DIMENSIONS];
    char attributes_values_char[CMOR_MAX_ATTRIBUTES][CMOR_MAX_STRING];
    double attributes_values_num[CMOR_MAX_ATTRIBUTES];
    char attributes_type[CMOR_MAX_ATTRIBUTES];	/*stores attributes type */
    char attributes[CMOR_MAX_ATTRIBUTES][CMOR_MAX_STRING];	/*stores attributes names */
    int nattributes;		/* number of  attributes */
    char type;
    char itype;
    double missing;
    double omissing;
    double tolerance;
    float valid_min;
    float valid_max;
    float ok_min_mean_abs;
    float ok_max_mean_abs;
    char chunking_dimensions[CMOR_MAX_STRING];
    int shuffle;
    int deflate;
    int deflate_level;
    int zstandard_level;
    int quantize_mode;
    int quantize_nsd;
    int nomissing;
    char iunits[CMOR_MAX_STRING];
    char ounits[CMOR_MAX_STRING];
    int isbounds;
    int needsinit;		/* need to be init or associated to file */
    int zaxis;			/* for z vars, associated axis stored here */
    double *values;
    double first_time;
    double last_time;
    double first_bound;
    double last_bound;
    char base_path[CMOR_MAX_STRING];
    char current_path[CMOR_MAX_STRING];
    char suffix[CMOR_MAX_STRING];
    int suffix_has_date;
    char frequency[CMOR_MAX_STRING];
    char branded_variable[CMOR_MAX_STRING];
    char branding_suffix[CMOR_MAX_STRING];
    char temporal_label[CMOR_MAX_STRING];
    char vertical_label[CMOR_MAX_STRING];
    char horizontal_label[CMOR_MAX_STRING];
    char area_label[CMOR_MAX_STRING];
} cmor_var_t;

extern cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
extern cmor_var_t cmor_formula[CMOR_MAX_FORMULA];

typedef struct cmor_mappings_ {
    int nattributes;
    char id[CMOR_MAX_STRING];
    char attributes_names[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    char coordinates[CMOR_MAX_STRING];
} cmor_mappings_t;

typedef struct {
    char key[CMOR_MAX_STRING];
    char *value;
} t_symstruct;

enum cmor_table_type {
    CMOR_TABLE_VARIABLE,
    CMOR_TABLE_CV,
    CMOR_TABLE_FORMULA,
    CMOR_TABLE_AXIS
};

typedef struct cmor_table_ {
    int id;
    int nvars;
    int nformula;
    int naxes;
    int nexps;
    int nmappings;
    float cf_version;
    char cmor_version[CMOR_MAX_STRING];
    char mip_era[CMOR_MAX_STRING];
    char checksum[CMOR_MAX_STRING];
    char Conventions[CMOR_MAX_STRING];
    char data_specs_version[CMOR_MAX_STRING];
    char szTable_id[CMOR_MAX_STRING];
    char expt_ids[CMOR_MAX_ELEMENTS][CMOR_MAX_STRING];
    char sht_expt_ids[CMOR_MAX_ELEMENTS][CMOR_MAX_STRING];
    char date[CMOR_MAX_STRING];
    cmor_axis_def_t axes[CMOR_MAX_ELEMENTS];
    cmor_var_def_t vars[CMOR_MAX_ELEMENTS];
    cmor_var_def_t formula[CMOR_MAX_FORMULA];
    cmor_mappings_t mappings[CMOR_MAX_ELEMENTS];
    cmor_CV_def_t *CV;
    char positive;
    char type;
    float valid_min;
    float valid_max;
    float ok_min_mean_abs;
    float ok_max_mean_abs;
    double missing_value;
    long    int_missing_value;
    double interval;
    double interval_warning;
    double interval_error;
    char URL[CMOR_MAX_STRING];
    char product[CMOR_MAX_STRING];
    char realm[CMOR_MAX_STRING];
    char path[CMOR_MAX_STRING];
    char **forcings;
    int nforcings;
    unsigned char md5[16];
    char generic_levels[CMOR_MAX_ELEMENTS][CMOR_MAX_STRING];
} cmor_table_t;

extern cmor_table_t cmor_tables[CMOR_MAX_TABLES];

//extern const char cmor_tracking_prefix_project_filter[CMOR_MAX_TRACKING_PREFIX_PROJECT_FILTER][CMOR_MAX_STRING];

typedef struct  attributes {
    char names[CMOR_MAX_STRING];
    char values[CMOR_MAX_STRING];
} attributes_def;

typedef struct cmor_dataset_def_ {
    char outpath[CMOR_MAX_STRING];
    char conventions[CMOR_MAX_STRING];

    char activity_id[CMOR_MAX_STRING];
    char tracking_prefix[CMOR_MAX_STRING];
    int nattributes;
    attributes_def attributes[CMOR_MAX_ATTRIBUTES];
  //  int realization;
    int leap_year;
    int leap_month;
    int month_lengths[12];
    int initiated;
    int associate_file;		/*flag to store associated variables separately */
    int associated_file;	/* ncid of associated file */
    int default_path_template; /* 1 if default path template is in use */
    int default_file_template; /* 1 if default file name template is in use */
    char associated_file_name[CMOR_MAX_STRING];	/*associated file path */
    char tracking_id[CMOR_MAX_STRING];	/*associated tracking id */
    char path_template[CMOR_MAX_STRING]; /* <keys> for each directory */
    char file_template[CMOR_MAX_STRING]; /* <keys> for filename */
    char furtherinfourl[CMOR_MAX_STRING]; /* further URL INFO template */
    char finalfilename[CMOR_MAX_STRING]; /* Final output file */
    char history_template[CMOR_MAX_STRING];
} cmor_dataset_def;

extern cmor_dataset_def cmor_current_dataset;

/* Now the funcs declarations */
#include "cmor_func_def.h"
#endif
