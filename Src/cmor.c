#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <json-c/json_tokener.h>
#include "cmor.h"
#include "cmor_locale.h"
#include <netcdf.h>
#include <netcdf_filter.h>
#include <udunits2.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <regex.h>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

/* ==================================================================== */
/*      this is defining NETCDF4 variable if we are                     */
/*      using NETCDF3 not used anywhere else                            */
/* ==================================================================== */

#ifndef NC_NETCDF4
#define NC_NETCDF4 0
#define NC_CLASSIC_MODEL 0
int nc_def_var_deflate(int i, int j, int k, int l, int m)
{
    return (0);
};

int nc_def_var_chunking(int i, int j, int k, size_t * l)
{
    return (0);
};
#endif

#ifndef H5Z_FILTER_ZSTD
int nc_def_var_zstandard(int i, int j, int k)
{
    return (0);
};
#endif

#ifndef NC_QUANTIZE_BITGROOM
int nc_def_var_quantize(int i, int j, int k, int l)
{
    return (0);
};
#endif

/* -------------------------------------------------------------------- */
/*      function declaration                                            */
/* -------------------------------------------------------------------- */
extern int cmor_set_cur_dataset_attribute_internal(char *name,
                                                   char *value, int optional);

extern int cmor_set_variable_attribute_internal(int id,
                                                char *attribute_name,
                                                char type, void *value);
extern int cmor_history_contains(int var_id, char *add);

extern void cdCompAdd(cdCompTime comptime,
                      double value, cdCalenType calendar, cdCompTime * result);

extern void cdCompAddMixed(cdCompTime ct, double value, cdCompTime * result);

/* -------------------------------------------------------------------- */
/*      Global Variable                                                 */
/* -------------------------------------------------------------------- */
int USE_NETCDF_4;
int cleanup_varid = -1;

const char CMOR_VALID_CALENDARS[CMOR_N_VALID_CALS][CMOR_MAX_STRING] =
  { "gregorian",
    "standard",
    "proleptic_gregorian",
    "noleap",
    "365_day",
    "360_day",
    "julian",
    "none"
};

int CMOR_HAS_BEEN_SETUP = 0;
int CMOR_TERMINATE_SIGNAL = -999;  /* not set by default */
int CV_ERROR = 0;
ut_system *ut_read = NULL;
FILE *output_logfile;

/* -------------------------------------------------------------------- */
/*      Variable related to cmor                                        */
/* -------------------------------------------------------------------- */
cmor_dataset_def cmor_current_dataset;
cmor_table_t cmor_tables[CMOR_MAX_TABLES];
cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
cmor_axis_t cmor_axes[CMOR_MAX_AXES];
cmor_grid_t cmor_grids[CMOR_MAX_GRIDS];

int CMOR_MODE;
int CMOR_TABLE;
int CMOR_VERBOSITY;
int CMOR_NETCDF_MODE;

int cmor_naxes;
int cmor_nvars;
int cmor_ntables;
int cmor_ngrids;

int cmor_nerrors;
int cmor_nwarnings;

int did_history = 0;

int CMOR_CREATE_SUBDIRECTORIES = 1;

char cmor_input_path[CMOR_MAX_STRING];
char cmor_traceback_info[CMOR_MAX_STRING];

int bAppendMode = 0;

volatile sig_atomic_t stop = 0;

/**************************************************************************/
/*                reset signal code                                       */
/**************************************************************************/
int cmor_get_terminate_signal() {
    return CMOR_TERMINATE_SIGNAL;
}
void cmor_set_terminate_signal_to_sigint() {
    CMOR_TERMINATE_SIGNAL = SIGINT;
}
void cmor_set_terminate_signal_to_sigterm() {
    CMOR_TERMINATE_SIGNAL = SIGTERM;
}
void cmor_set_terminate_signal( int code) {
    CMOR_TERMINATE_SIGNAL = code;
}
/**************************************************************************/
/*                cmor_mkdir()                                            */
/**************************************************************************/
void terminate(int signal)
{
    if (signal == CMOR_TERMINATE_SIGNAL) {
        stop = 1;
    }
}

/**************************************************************************/
/*                cmor_mkdir()                                            */
/**************************************************************************/
int cmor_mkdir(const char *dir)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    int ierr;
    cmor_add_traceback("cmor_mkdir");

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, (S_IRWXU | S_IRWXG | S_IRWXO));
            *p = '/';
        }
    }
    ierr = mkdir(tmp, (S_IRWXU | S_IRWXG | S_IRWXO));
    cmor_pop_traceback();
    return (ierr);
}

/**************************************************************************/
/*                                                                        */
/*                 cmorstringstring()                                     */
/*                                                                        */
/**************************************************************************/
int cmor_stringinstring(char *dest, char *src)
{
/* -------------------------------------------------------------------- */
/*      returns 1 if dest contains the words of src.                    */
/*      The end of a word is either a space, a period or null.          */
/*                                                                      */
/*      This will not give the desired results if                       */
/*      period is used internal to a word.                              */
/* -------------------------------------------------------------------- */
    char *pstr = dest;
    cmor_add_traceback("cmor_stringinstring");

    while ((pstr = strstr(pstr, src))) {
/* -------------------------------------------------------------------- */
/*      if this word is at the beginning of msg or preceeded by a       */
/*      space                                                           */
/* -------------------------------------------------------------------- */

        if (pstr == dest || pstr[-1] == ' ') {
/* -------------------------------------------------------------------- */
/*      if it isn't a substring match                                   */
/* -------------------------------------------------------------------- */
            if ((pstr[strlen(src)] == ' ') ||
                (pstr[strlen(src)] == 0) || (pstr[strlen(src)] == '.')) {
/* -------------------------------------------------------------------- */
/*      then return( 1 ) to indicate string found                          */
/* -------------------------------------------------------------------- */
                return (1);
            }
        }
        pstr++;                 /* In which case, skip to the next char */
    }
/* -------------------------------------------------------------------- */
/*      nothing like src is in dest, and so return the location of the end*/
/*      of string                                                       */
/* -------------------------------------------------------------------- */
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                                                                      */
/*      cmor_cat_unique_string()                                        */
/*                                                                      */
/************************************************************************/
void cmor_cat_unique_string(char *dest, char *src)
{
    int offset;
    int spare_space;
    cmor_add_traceback("cmor_cat_unique_string");

/* -------------------------------------------------------------------- */
/*      if this string is in msg                                        */
/* -------------------------------------------------------------------- */
    if (!cmor_stringinstring(dest, src)) {
        if ((offset = strlen(dest))) {
            strcat(dest + offset, " ");
            offset++;
            spare_space = CMOR_MAX_STRING - offset - 1;
            strncat(dest + offset, src, spare_space);
        } else {
            strncpy(dest, src, CMOR_MAX_STRING);
        }
    }
    cmor_pop_traceback();
}

/**************************************************************************/
/*                                                                        */
/*             cmor_check_forcing_validity()                              */
/*                                                                        */
/**************************************************************************/
int cmor_check_forcing_validity(int table_id, char *value)
{
    int i, j, n, msg_len, found = 0;
    char *msg;
    char astr[CMOR_MAX_STRING];
    char **bstr;

    cmor_add_traceback("cmor_check_forcing_validity");
    if (cmor_tables[table_id].nforcings == 0) {
        cmor_pop_traceback();
        return (0);
    }
    strcpy(astr, value);
    found = 0;

    for (i = 0; i < strlen(astr); i++) {
        if (astr[i] == ',')
            astr[i] = ' ';

/* -------------------------------------------------------------------- */
/*      removes everything  after first parenthesis                     */
/* -------------------------------------------------------------------- */
        if (astr[i] == '(')
            astr[i] = '\0';
    }
    cmor_convert_string_to_list(astr, 'c', (void **)&bstr, &n);
    if (n == 0) {
        cmor_pop_traceback();
        return (0);
    }
    for (i = 0; i < n; i++) {
        found = 0;
        for (j = 0; j < cmor_tables[table_id].nforcings; j++) {
            if (strcmp(bstr[i], cmor_tables[table_id].forcings[j]) == 0) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            msg_len = 0;
            for (j = 0; j < cmor_tables[table_id].nforcings; j++) {
                if(j == 0) {
                    msg_len += snprintf(NULL, 0, " %s",
                    cmor_tables[table_id].forcings[j]);
                } else {
                    msg_len += snprintf(NULL, 0, ", %s",
                    cmor_tables[table_id].forcings[j]);
                } 
            }
            msg_len += 1;
            msg = (char *)malloc(msg_len * sizeof(char));
            msg_len = 0;
            for (j = 0; j < cmor_tables[table_id].nforcings; j++) {
                if(j == 0) {
                    msg_len += sprintf(&msg[msg_len], "%s",
                    cmor_tables[table_id].forcings[j]);
                } else {
                    msg_len += sprintf(&msg[msg_len], ", %s",
                    cmor_tables[table_id].forcings[j]);
                } 
            }
            cmor_handle_error_variadic(
                "forcing attribute elt %i (%s) is not valid for\n! "
                "table %s, valid values are: %s",
                CMOR_NORMAL,
                i, bstr[i], cmor_tables[table_id].szTable_id, msg);
            free(msg);
            cmor_pop_traceback();
            return (-1);
        }
    }
/* -------------------------------------------------------------------- */
/*      Ok now we need to clean up the memory allocations....           */
/* -------------------------------------------------------------------- */
    for (i = 0; i < n; i++) {
        free(bstr[i]);
    }
    free(bstr);
    cmor_pop_traceback();
    return (0);
}

/**************************************************************************/
/*                  cmor_check_expt_id()                                  */
/**************************************************************************/
int cmor_check_expt_id(char *szExptID, int nTableID,
                       char *szGblAttLong, char *szGblAttShort)
{
    int i, j;
    char szTableExptID[CMOR_MAX_STRING];
    char szTableShtExptID[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_check_expt_id");
    j = 1;
    for (i = 0; i <= cmor_tables[nTableID].nexps; i++) {

        strncpy(szTableExptID,
                cmor_tables[nTableID].expt_ids[i], CMOR_MAX_STRING);

        strncpy(szTableShtExptID,
                cmor_tables[nTableID].sht_expt_ids[i], CMOR_MAX_STRING);

        if ((strncmp(szTableExptID, szExptID, CMOR_MAX_STRING) == 0) ||
            (strncmp(szTableShtExptID, szExptID, CMOR_MAX_STRING) == 0)) {
            j = 0;
            cmor_set_cur_dataset_attribute_internal(szGblAttLong,
                                                    szTableExptID, 0);
            cmor_set_cur_dataset_attribute_internal(szGblAttShort,
                                                    szTableShtExptID, 1);
            strncpy(szExptID, szTableShtExptID, CMOR_MAX_STRING);
            break;
        }
    }

    cmor_pop_traceback();

    return (j);
}

/************************************************************************/
/*                            strncpytrim()                             */
/************************************************************************/
int strncpytrim(char *out, char *in, int max)
{

    int i, n, j, k;

    cmor_add_traceback("strncpytrim");
    j = 0;
    n = strlen(in);

    if (n > max) {
        n = max;
    }

    /* -------------------------------------------------------------------- */
    /*      Look for first space position and last space position and       */
    /*      copy interval characters in out.                                */
    /* -------------------------------------------------------------------- */
    while ((in[j] == ' ') && (j < n)) {
        j++;
    }

    k = n - 1;

    while ((in[k] == ' ') && (k > 0)) {
        k--;
    }

    for (i = j; i <= k; i++) {
        out[i - j] = in[i];
    }

    out[i - j] = '\0';

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                           cmor_is_setup()                            */
/************************************************************************/
void cmor_is_setup(void)
{

    extern int CMOR_HAS_BEEN_SETUP;

    stop = 0;
    cmor_add_traceback("cmor_is_setup");

    if (CMOR_HAS_BEEN_SETUP == 0) {
        cmor_handle_error_variadic(
            "You need to run cmor_setup before calling any cmor_function",
            CMOR_NOT_SETUP);
    }
    cmor_pop_traceback();
    return;
}

/************************************************************************/
/*                         cmor_add_traceback()                         */
/************************************************************************/
void cmor_add_traceback(char *name)
{
    char tmp[CMOR_MAX_STRING];

    if (strlen(cmor_traceback_info) == 0) {
        sprintf(cmor_traceback_info, "%s\n! ", name);
    } else {
        sprintf(tmp, "%s\n! called from: %s", name, cmor_traceback_info);
        strncpy(cmor_traceback_info, tmp, CMOR_MAX_STRING);
    }
    return;
}

/************************************************************************/
/*                         cmor_pop_traceback()                         */
/************************************************************************/
void cmor_pop_traceback(void)
{
    int i;
    char tmp[CMOR_MAX_STRING];

    strcpy(tmp, "");
    for (i = 0; i < strlen(cmor_traceback_info); i++) {
        if (strncmp(&cmor_traceback_info[i], "called from: ", 13) == 0) {
            strcpy(tmp, &cmor_traceback_info[i + 13]);
            break;
        }
    }
    strcpy(cmor_traceback_info, tmp);
    return;
}

/************************************************************************/
/*                         cmor_have_NetCDF4()                          */
/************************************************************************/
int cmor_have_NetCDF4(void)
{
    char version[50];
    int major;

    cmor_pop_traceback();
    strncpy(version, nc_inq_libvers(), 50);
    sscanf(version, "%1d%*s", &major);
    if (major != 4) {
        cmor_pop_traceback();
        return (1);
    }
    cmor_pop_traceback();

    return (0);
}

/************************************************************************/
/*                          cmor_prep_units()                           */
/************************************************************************/
int cmor_prep_units(char *uunits,
                    char *cunits,
                    ut_unit ** user_units,
                    ut_unit ** cmor_units, cv_converter ** ut_cmor_converter)
{

    extern ut_system *ut_read;
    char local_unit[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_prep_units");
    cmor_is_setup();
    *cmor_units = ut_parse(ut_read, cunits, UT_ASCII);
    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: analyzing units from cmor (%s)",
            CMOR_CRITICAL, cunits);
        cmor_pop_traceback();
        return (1);
    }

    strncpy(local_unit, uunits, CMOR_MAX_STRING);
    ut_trim(local_unit, UT_ASCII);
    *user_units = ut_parse(ut_read, local_unit, UT_ASCII);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: analyzing units from user (%s)",
            CMOR_CRITICAL, local_unit);
        cmor_pop_traceback();
        return (1);
    }

    if (ut_are_convertible(*cmor_units, *user_units) == 0) {
        cmor_handle_error_variadic(
            "Udunits: cmor and user units are incompatible: %s and %s",
            CMOR_CRITICAL, cunits, uunits);
        cmor_pop_traceback();
        return (1);
    }

    *ut_cmor_converter = ut_get_converter(*user_units, *cmor_units);

    if (*ut_cmor_converter == NULL) {
    }

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error getting converter from %s to %s",
            CMOR_CRITICAL, cunits, local_unit);
        cmor_pop_traceback();
        return (1);
    }

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_have_NetCDF41min()                        */
/************************************************************************/
int cmor_have_NetCDF41min(void)
{
    char version[50];
    int major, minor;

    cmor_add_traceback("cmor_have_NetCDF41min");
    strncpy(version, nc_inq_libvers(), 50);
    sscanf(version, "%1d%*c%1d%*s", &major, &minor);
    if (major > 4) {
        cmor_pop_traceback();
        return (0);
    }
    if (major < 4) {
        cmor_pop_traceback();
        return (1);
    }
    if (minor < 1) {
        cmor_pop_traceback();
        return (1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                         cmor_handle_error_internal()                          */
/************************************************************************/
void cmor_handle_error_internal(char *error_msg, int level)
{
    int i;
    extern FILE *output_logfile;

    if (output_logfile == NULL)
        output_logfile = stderr;

    if (CMOR_VERBOSITY != CMOR_QUIET) {
        fprintf(output_logfile, "\n");
    }
    if (level == CMOR_WARNING) {
        cmor_nwarnings++;
        if (CMOR_VERBOSITY != CMOR_QUIET) {

#ifdef COLOREDOUTPUT
            fprintf(output_logfile, "%c[%d;%d;%dm", 0X1B, 2, 34, 47);
#endif

            fprintf(output_logfile, "C Traceback:\nIn function: %s",
                    cmor_traceback_info);

#ifdef COLOREDOUTPUT
            fprintf(output_logfile, "%c[%dm", 0X1B, 0);
#endif

            fprintf(output_logfile, "\n\n");

#ifdef COLOREDOUTPUT
            fprintf(output_logfile, "%c[%d;%d;%dm", 0X1B, 1, 34, 47);
#endif
        }
    } else {
        cmor_nerrors++;

#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%d;%d;%dm", 0X1B, 2, 31, 47);
#endif

        fprintf(output_logfile, "C Traceback:\n! In function: %s",
                cmor_traceback_info);

#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%dm", 0X1B, 0);
#endif

        fprintf(output_logfile, "\n\n");

#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%d;%d;%dm", 0X1B, 1, 31, 47);
#endif
    }
    // fprintf(stderr, "%s ERROR LEVEL %d\n", error_msg, level);
    if (CMOR_VERBOSITY != CMOR_QUIET || level != CMOR_WARNING) {
        for (i = 0; i < 25; i++) {
            fprintf(output_logfile, "!");
        }
        fprintf(output_logfile, "\n");
        fprintf(output_logfile, "!\n");
        
        if (level == CMOR_WARNING)
            fprintf(output_logfile, "! Warning: %s\n", error_msg);
        else
            fprintf(output_logfile, "! Error: %s\n", error_msg);

        fprintf(output_logfile, "!\n");

        for (i = 0; i < 25; i++)
            fprintf(output_logfile, "!");

#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%dm", 0X1B, 0);
#endif

        fprintf(output_logfile, "\n\n");
    }

    CV_ERROR = 1;
    if (level == CMOR_NOT_SETUP) {
        exit(1);

    }
    if ((CMOR_MODE == CMOR_EXIT_ON_WARNING) || (level == CMOR_CRITICAL)) {
        fflush(stdout); 
        fflush(output_logfile); 
        kill(getpid(), SIGTERM);
    }
    fflush(output_logfile);
}

void cmor_handle_error(char *error_msg, int level)
{
    cmor_handle_error_internal(error_msg, level);
}

void cmor_handle_error_variadic(char *error_msg, int level, ...)
{
    va_list args;
    size_t size;
    char *msg;

    if (output_logfile == NULL)
        output_logfile = stderr;

    va_start (args, level);
    size = vsnprintf(NULL, 0, error_msg, args);
    va_end (args);

    size++;
    msg = (char *)malloc(size*sizeof(char));

    va_start (args, level);
    vsnprintf(msg, size, error_msg, args);
    va_end (args);

    cmor_handle_error_internal(msg, level);

    free(msg);
}

void cmor_handle_error_var(char *error_msg, int level, int var_id)
{
    cmor_vars[var_id].error = 1;
    cmor_handle_error_internal(error_msg, level);
}

void cmor_handle_error_var_variadic(char *error_msg, int level, int var_id, ...)
{
    va_list args;
    size_t size;
    char *msg;

    if (output_logfile == NULL)
        output_logfile = stderr;

    va_start (args, var_id);
    size = vsnprintf(NULL, 0, error_msg, args);
    va_end (args);

    size++;
    msg = (char *)malloc(size*sizeof(char));

    va_start (args, var_id);
    vsnprintf(msg, size, error_msg, args);
    va_end (args);

    cmor_vars[var_id].error = 1;
    cmor_handle_error_internal(msg, level);

    free(msg);
}

/************************************************************************/
/*                        cmor_reset_variable()                         */
/************************************************************************/
void cmor_reset_variable(int var_id)
{
    extern cmor_var_t cmor_vars[];
    int j;

    cmor_vars[var_id].self = -1;
    cmor_vars[var_id].grid_id = -1;
    cmor_vars[var_id].sign = 1;
    cmor_vars[var_id].zfactor = -1;
    cmor_vars[var_id].ref_table_id = -1;
    cmor_vars[var_id].ref_var_id = -1;
    cmor_vars[var_id].initialized = -1;
    cmor_vars[var_id].error = 0;
    cmor_vars[var_id].closed = 0;
    cmor_vars[var_id].nc_var_id = -999;

    for (j = 0; j < CMOR_MAX_VARIABLES; j++) {
        cmor_vars[var_id].nc_zfactors[j] = -999;
    }

    cmor_vars[var_id].nzfactor = 0;
    cmor_vars[var_id].ntimes_written = 0;

    for (j = 0; j < 10; j++) {
        cmor_vars[var_id].ntimes_written_coords[j] = -1;
        cmor_vars[var_id].associated_ids[j] = -1;
        cmor_vars[var_id].ntimes_written_associated[j] = 0;
    }

    cmor_vars[var_id].time_nc_id = -999;
    cmor_vars[var_id].time_bnds_nc_id = -999;
    cmor_vars[var_id].id[0] = '\0';
    cmor_vars[var_id].ndims = 0;

    for (j = 0; j < CMOR_MAX_DIMENSIONS; j++) {
/* -------------------------------------------------------------------- */
/*      place holder for singleton axes ids                             */
/* -------------------------------------------------------------------- */
        cmor_vars[var_id].singleton_ids[j] = -1;
        cmor_vars[var_id].axes_ids[j] = -1;
        cmor_vars[var_id].original_order[j] = -1;

    }

    for (j = 0; j < CMOR_MAX_ATTRIBUTES; j++) {
        cmor_vars[var_id].attributes_values_char[j][0] = '\0';
        cmor_vars[var_id].attributes_values_num[j] = -999.;
        cmor_vars[var_id].attributes_type[j] = '\0';
        cmor_vars[var_id].attributes[j][0] = '\0';
    }

    cmor_vars[var_id].nattributes = 0;
    cmor_vars[var_id].type = '\0';
    cmor_vars[var_id].itype = 'N';
    cmor_vars[var_id].missing = 1.e20;
    cmor_vars[var_id].omissing = 1.e20;
    cmor_vars[var_id].tolerance = 1.e-4;
    cmor_vars[var_id].valid_min = 1.e20;
    cmor_vars[var_id].valid_max = 1.e20;
    cmor_vars[var_id].ok_min_mean_abs = 1.e20;
    cmor_vars[var_id].ok_max_mean_abs = 1.e20;
    cmor_vars[var_id].shuffle = 0;
    cmor_vars[var_id].deflate = 1;
    cmor_vars[var_id].deflate_level = 1;
    cmor_vars[var_id].zstandard_level = 3;
    cmor_vars[var_id].quantize_mode = 0;
    cmor_vars[var_id].quantize_nsd = 1;
    cmor_vars[var_id].nomissing = 1;
    cmor_vars[var_id].iunits[0] = '\0';
    cmor_vars[var_id].ounits[0] = '\0';
    cmor_vars[var_id].isbounds = 0;
    cmor_vars[var_id].needsinit = 1;
    cmor_vars[var_id].zaxis = -1;

    if (cmor_vars[var_id].values != NULL) {
        free(cmor_vars[var_id].values);
    }

    cmor_vars[var_id].values = NULL;
    cmor_vars[var_id].first_time = -999.;
    cmor_vars[var_id].last_time = -999.;
    cmor_vars[var_id].first_bound = 1.e20;
    cmor_vars[var_id].last_bound = 1.e20;
    cmor_vars[var_id].base_path[0] = '\0';
    cmor_vars[var_id].current_path[0] = '\0';
    cmor_vars[var_id].suffix[0] = '\0';
    cmor_vars[var_id].suffix_has_date = 0;
    cmor_vars[var_id].frequency[0] = '\0';
}

/************************************************************************/
/*                             cmor_setup()                             */
/************************************************************************/
int cmor_setup(char *path,
               int *netcdf,
               int *verbosity,
               int *mode, char *logfile, int *create_subdirectories)
{

    extern cmor_axis_t cmor_axes[];
    extern int CMOR_TABLE, cmor_ntables;
    extern ut_system *ut_read;
    extern cmor_dataset_def cmor_current_dataset;

    ut_unit *dimlessunit = NULL, *perunit = NULL, *newequnit = NULL;
    ut_status myutstatus;
    ut_unit *PracticalSSunit = NULL;
    ut_unit *pss78unit = NULL;
    ut_unit *psuunit = NULL;

    int i, j;
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];
    char tmplogfile[CMOR_MAX_STRING];
    time_t lt;

    struct stat buf;
    struct tm *ptr;
    extern FILE *output_logfile;
    extern int did_history;
    struct sigaction action;


    /*********************************************************/
    /* set the default signal */
    /*********************************************************/

    if (CMOR_TERMINATE_SIGNAL == -999) {
        CMOR_TERMINATE_SIGNAL = SIGTERM;
    }

    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate;
    sigaction(CMOR_TERMINATE_SIGNAL, &action, NULL);

    strcpy(cmor_traceback_info, "");
    cmor_add_traceback("cmor_setup");

/* -------------------------------------------------------------------- */
/*      ok we need to know if we are using NC3 or 4                     */
/* -------------------------------------------------------------------- */
    USE_NETCDF_4 = -1;
    if (cmor_have_NetCDF4() == 0) {
        USE_NETCDF_4 = 1;
        if (cmor_have_NetCDF41min() != 0) {
            cmor_handle_error_variadic(
                    "You are using a wrong version of NetCDF4 (%s), \n! "
                    "you need 4.1",
                    CMOR_CRITICAL, nc_inq_libvers());
        }
    } else {
        cmor_handle_error_variadic(
                "You are using a wrong version of NetCDF (%s), you need \n! "
                "4.1 or an earlier netCDF release.",
                CMOR_CRITICAL, nc_inq_libvers());
    }
    did_history = 0;
    CMOR_HAS_BEEN_SETUP = 1;
    CMOR_TABLE = -1;
    cmor_ngrids = -1;
    cmor_nvars = -1;
    cmor_naxes = -1;
    cmor_ntables = -1;
    cmor_nerrors = 0;
    cmor_nwarnings = 0;

    // Define mode
    if (mode == NULL) {
        CMOR_MODE = CMOR_NORMAL;

    } else {

        if (*mode != CMOR_EXIT_ON_WARNING && *mode != CMOR_EXIT_ON_MAJOR
            && *mode != CMOR_NORMAL) {

            cmor_handle_error_variadic(
                     "exit mode can be either CMOR_EXIT_ON_WARNING CMOR_NORMAL "
                     "or CMOR_EXIT_ON_MAJOR", CMOR_CRITICAL);
        }

        CMOR_MODE = *mode;

    }
    // Define verbosity
    if (verbosity == NULL) {
        CMOR_VERBOSITY = CMOR_NORMAL;

    } else {

        if (*verbosity != CMOR_QUIET && *verbosity != CMOR_NORMAL) {
            cmor_handle_error_variadic(
                     "verbosity mode can be either CMOR_QUIET or CMOR_NORMAL",
                     CMOR_NORMAL);
        }
        CMOR_VERBOSITY = *verbosity;
    }

    if (logfile == NULL) {
        output_logfile = NULL;
    } else {

#ifdef COLOREDOUTPUT
#undef COLOREDOUTPUT
#endif
        cmor_trim_string(logfile, tmplogfile);
        output_logfile = NULL;
        output_logfile = fopen(tmplogfile, "r");
        if (output_logfile != NULL) {
/* -------------------------------------------------------------------- */
/*      logfile already exists need to rename it                        */
/*      Figure out the time                                             */
/* -------------------------------------------------------------------- */
            stat(tmplogfile, &buf);
            lt = buf.st_ctime;
            ptr = localtime(&lt);

            snprintf(msg, CMOR_MAX_STRING, "%s_%.4i-%.2i-%.2iT%.2i:%.2i:%.2i",
                     tmplogfile, ptr->tm_year + 1900, ptr->tm_mon + 1,
                     ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);

            fclose(output_logfile);
            rename(tmplogfile, msg);

            output_logfile = NULL;
            output_logfile = fopen(tmplogfile, "w");

            if (output_logfile == NULL) {
                cmor_handle_error_variadic(
                         "Could not open logfile %s for writing",
                         CMOR_CRITICAL, tmplogfile);
            }
            cmor_handle_error_variadic(
                "Logfile %s already exist.\n! Renamed to: %s",
                CMOR_WARNING, tmplogfile, msg);
        } else {
            output_logfile = fopen(tmplogfile, "w");

            if (output_logfile == NULL) {
                cmor_handle_error_variadic(
                    "Could not open logfile %s for writing",
                    CMOR_CRITICAL, tmplogfile);
            }
        }
    }

    if (netcdf == NULL) {
        CMOR_NETCDF_MODE = CMOR_PRESERVE;

    } else {
        if (*netcdf != CMOR_PRESERVE_4 && *netcdf != CMOR_APPEND_4
            && *netcdf != CMOR_REPLACE_4 && *netcdf != CMOR_PRESERVE_3
            && *netcdf != CMOR_APPEND_3 && *netcdf != CMOR_REPLACE_3) {
            cmor_handle_error_variadic(
                     "file mode can be either CMOR_PRESERVE, CMOR_APPEND, "
                     "CMOR_REPLACE, CMOR_PRESERVE_4, CMOR_APPEND_4, "
                     "CMOR_REPLACE_4, CMOR_PRESERVE_3, CMOR_APPEND_3 or "
                     "CMOR_REPLACE_3", CMOR_CRITICAL);
        }
        CMOR_NETCDF_MODE = *netcdf;

    }

    /* Make sure we are not trying to use NETCDF4 mode while linked against NetCDF3 */
    if (((CMOR_NETCDF_MODE == CMOR_PRESERVE_4)
         || (CMOR_NETCDF_MODE == CMOR_REPLACE_4)
         || (CMOR_NETCDF_MODE == CMOR_APPEND_4)) && (USE_NETCDF_4 == 0)) {
        cmor_handle_error_variadic(
                "You are trying to use a NetCDF4 mode but linked against "
                "NetCDF3 libraries", CMOR_CRITICAL);

    }

    if ((path == NULL) || (strcmp(path, "") == 0)) {
        strncpy(cmor_input_path, ".", CMOR_MAX_STRING);
    } else {
        strncpytrim(cmor_input_path, path, CMOR_MAX_STRING);
    }

    for (i = 0; i < CMOR_MAX_VARIABLES; i++) {
        cmor_reset_variable(i);
    }

    for (i = 0; i < CMOR_MAX_AXES; i++) {
        cmor_axes[i].ref_table_id = -1;
        cmor_axes[i].ref_axis_id = -1;
        cmor_axes[i].isgridaxis = -1;
        cmor_axes[i].axis = '\0';
        cmor_axes[i].iunits[0] = '\0';
        cmor_axes[i].id[0] = '\0';

        if (cmor_axes[i].values != NULL)
            free(cmor_axes[i].values);

        cmor_axes[i].values = NULL;

        if (cmor_axes[i].bounds != NULL)
            free(cmor_axes[i].bounds);

        cmor_axes[i].bounds = NULL;

        if (cmor_axes[i].cvalues != NULL) {
            for (j = 0; j < cmor_axes[i].length; j++) {
                if (cmor_axes[i].cvalues[j] != NULL) {
                    free(cmor_axes[i].cvalues[j]);
                    cmor_axes[i].cvalues[j] = NULL;
                }
            }
            free(cmor_axes[i].cvalues);
        }

        cmor_axes[i].cvalues = NULL;
        cmor_axes[i].length = 0;
        cmor_axes[i].revert = 1;        /* 1 means no reverse -1 means reverse */
        cmor_axes[i].offset = 0;
        cmor_axes[i].type = '\0';

        for (j = 0; j < CMOR_MAX_ATTRIBUTES; j++) {
            cmor_axes[i].attributes_values_char[j][0] = '\0';
            cmor_axes[i].attributes_values_num[j] = -999.;
            cmor_axes[i].attributes_type[j] = '\0';
            cmor_axes[i].attributes[j][0] = '\0';
        }

        cmor_axes[i].nattributes = 0;
        cmor_axes[i].hybrid_in = 0;
        cmor_axes[i].hybrid_out = 0;
        cmor_axes[i].store_in_netcdf = 1;
        cmor_axes[i].wrapping = NULL;
        cmor_set_axis_attribute(i, "units", 'c', "");
        cmor_set_axis_attribute(i, "interval", 'c', "");
    }

    if (create_subdirectories != NULL) {
        CMOR_CREATE_SUBDIRECTORIES = *create_subdirectories;
    }

    if ((CMOR_CREATE_SUBDIRECTORIES != 1)
        && (CMOR_CREATE_SUBDIRECTORIES != 0)) {
        cmor_handle_error_variadic(
                 "cmor_setup: create_subdirectories must be 0 or 1",
                 CMOR_CRITICAL);
    }

/* -------------------------------------------------------------------- */
/*      initialize the udunits                                          */
/* -------------------------------------------------------------------- */
    if (ut_read != NULL) {
        ut_free_system(ut_read);
    }

    ut_set_error_message_handler(ut_ignore);

    ut_read = ut_read_xml(NULL);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error reading units system", CMOR_CRITICAL);
    }

    ut_set_error_message_handler(ut_ignore);

    if (newequnit != NULL) {
        ut_free(newequnit);
    }

    newequnit = ut_new_base_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: creating dimlessnew base unit", CMOR_CRITICAL);
    }

    myutstatus = ut_map_name_to_unit("eq", UT_ASCII, newequnit);
    ut_free(newequnit);

    if (myutstatus != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error mapping dimless 'eq' unit", CMOR_CRITICAL);
    }

    if (dimlessunit != NULL)
        ut_free(dimlessunit);

    dimlessunit = ut_new_dimensionless_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: creating dimless unit", CMOR_CRITICAL);
    }

    myutstatus = ut_map_name_to_unit("dimless", UT_ASCII, dimlessunit);

    if (myutstatus != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error mapping dimless unit", CMOR_CRITICAL);
    }

    if (perunit != NULL)
        ut_free(perunit);

    perunit = ut_scale(.01, dimlessunit);
    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error creating percent unit", CMOR_CRITICAL);
    }
    myutstatus = ut_map_name_to_unit("%", UT_ASCII, perunit);
    if (myutstatus != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error mapping percent unit", CMOR_CRITICAL);
    }
    // -----------------------
    // Create "psu" unit
    // -----------------------
    if (psuunit != NULL)
        ut_free(psuunit);

    psuunit = ut_new_dimensionless_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: creating psuunit unit", CMOR_CRITICAL);
    }

    if (perunit != NULL)
        ut_free(perunit);
    perunit = ut_scale(.001, psuunit);
    myutstatus = ut_map_name_to_unit("psu", UT_ASCII, perunit);

    if (myutstatus != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error mapping psu unit", CMOR_CRITICAL);
    }
    // -----------------------
    // Create "PSS-78" unit
    // -----------------------
    if (pss78unit != NULL)
        ut_free(pss78unit);

    pss78unit = ut_new_dimensionless_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: creating dimless unit", CMOR_CRITICAL);
    }

    if (perunit != NULL)
        ut_free(perunit);
    perunit = ut_scale(.001, pss78unit);
    myutstatus = ut_map_name_to_unit("PSS", UT_UTF8, perunit);

    if (myutstatus != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error mapping PSS-78 unit", CMOR_CRITICAL);
    }
    // -----------------------
    // Create "Practical Salinity Scale 78" unit
    // -----------------------
    if (PracticalSSunit != NULL)
        ut_free(PracticalSSunit);

    PracticalSSunit = ut_new_dimensionless_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: creating Practical Salinity Scale 78 unit",
            CMOR_CRITICAL);
    }
    if (perunit != NULL)
        ut_free(perunit);
    perunit = ut_scale(.001, PracticalSSunit);
    myutstatus =
      ut_map_name_to_unit("practical_salinity_scale_", UT_UTF8, perunit);

    if (myutstatus != UT_SUCCESS) {
        cmor_handle_error_variadic(
            "Udunits: Error mapping Practical Salinity Scale 78 unit",
            CMOR_CRITICAL);
    }
    ut_free(PracticalSSunit);
    ut_free(pss78unit);
    ut_free(psuunit);
    ut_free(dimlessunit);
    ut_free(perunit);

/* -------------------------------------------------------------------- */
/*      initialized dataset                                             */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_ATTRIBUTES; i++) {
        cmor_current_dataset.attributes[i].names[0] = '\0';
        cmor_current_dataset.attributes[i].values[0] = '\0';
    }
    cmor_current_dataset.nattributes = 0;
    cmor_current_dataset.leap_year = 0;
    cmor_current_dataset.leap_month = 0;
    cmor_current_dataset.associate_file = 0;
    cmor_current_dataset.associated_file = -1;
/* -------------------------------------------------------------------- */
/*      generates a unique id                                           */
/* -------------------------------------------------------------------- */
    cmor_generate_uuid();
    strncpy(cmor_current_dataset.associated_file_name, "", CMOR_MAX_STRING);
    strncpy(cmor_current_dataset.finalfilename, "", CMOR_MAX_STRING);

    for (i = 0; i < 12; i++)
        cmor_current_dataset.month_lengths[i] = 0;
    cmor_current_dataset.initiated = 0;

    for (i = 0; i < CMOR_MAX_GRIDS; i++) {
        strncpy(cmor_grids[i].mapping, "", CMOR_MAX_STRING);
        cmor_grids[i].ndims = 0;
        cmor_grids[i].nattributes = 0;

        for (j = 0; j < CMOR_MAX_GRID_ATTRIBUTES; j++) {
            cmor_grids[i].attributes_values[j] = 1.e20;
            cmor_grids[i].attributes_names[j][0] = '\0';
        }

        if (cmor_grids[i].lats != NULL)
            free(cmor_grids[i].lats);
        if (cmor_grids[i].lons != NULL)
            free(cmor_grids[i].lons);
        if (cmor_grids[i].blats != NULL)
            free(cmor_grids[i].blats);
        if (cmor_grids[i].blons != NULL)
            free(cmor_grids[i].blons);

        cmor_grids[i].lats = NULL;
        cmor_grids[i].lons = NULL;
        cmor_grids[i].blats = NULL;
        cmor_grids[i].blons = NULL;

        cmor_grids[i].istimevarying = 0;
        cmor_grids[i].nvertices = 0;

        for (j = 0; j < 6; j++)
            cmor_grids[i].associated_variables[j] = -1;
    }

    cmor_pop_traceback();
    return (0);

}

/************************************************************************/
/*                     cmor_open_inpathFile()                           */
/************************************************************************/
json_object *cmor_open_inpathFile(char *szFilename)
{
    FILE *table_file;
    char szFullName[CMOR_MAX_STRING];
    char *buffer;
    int nFileSize;
    json_object *oJSON;

    cmor_add_traceback("cmor_open_inpathFile");

    /* -------------------------------------------------------------------- */
    /*      Try to find file in current directory or in cmor_iput_path      */
    /* -------------------------------------------------------------------- */
    strcpy(szFullName, szFilename);
    table_file = fopen(szFullName, "r");
    int read_size;

    if (table_file == NULL) {
        if (szFilename[0] != '/') {
            snprintf(szFullName, CMOR_MAX_STRING, "%s/%s", cmor_input_path,
                     szFilename);
            table_file = fopen(szFullName, "r");
        }

        if (table_file == NULL) {
            cmor_handle_error_variadic(
                "Could not find file: %s",
                CMOR_NORMAL, szFilename);
            cmor_ntables -= 1;
            cmor_pop_traceback();
            return (NULL);
        }
    }

/* -------------------------------------------------------------------- */
/*      Read the entire file in memory                                 */
/* -------------------------------------------------------------------- */
    fseek(table_file, 0, SEEK_END);
    nFileSize = ftell(table_file);
    rewind(table_file);
    buffer = (char *)malloc(sizeof(char) * (nFileSize + 1));
    read_size = fread(buffer, sizeof(char), nFileSize, table_file);
    buffer[nFileSize] = '\0';

/* -------------------------------------------------------------------- */
/*      print error and exist if this is not a JSON file                */
/* -------------------------------------------------------------------- */
    if (buffer[0] != '{') {
        free(buffer);
        buffer = NULL;
        cmor_handle_error_variadic(
            "Could not understand file \"%s\" Is this a JSON CMOR table?",
            CMOR_CRITICAL, szFullName);
        cmor_ntables--;
        cmor_pop_traceback();
        return (NULL);
    }
/* -------------------------------------------------------------------- */
/*      print error and exit if file was not completely read            */
/* -------------------------------------------------------------------- */
    if (nFileSize != read_size) {
        free(buffer);
        buffer = NULL;
        cmor_handle_error_variadic(
            "Could not read file %s check file permission",
            CMOR_CRITICAL, szFullName);
        cmor_ntables--;
        cmor_pop_traceback();
        return (NULL);
    }

/* -------------------------------------------------------------------- */
/*     parse buffer into json object                                    */
/* -------------------------------------------------------------------- */
    oJSON = json_tokener_parse(buffer);
    if (oJSON == NULL) {
        cmor_handle_error_variadic(
                 "Please validate JSON File!\n! "
                 "USE: http://jsonlint.com/\n! "
                 "Syntax Error in file: %s\n!  " "%s",
                 CMOR_CRITICAL, szFullName, buffer);
    }
    cmor_pop_traceback();
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    if (table_file != NULL) {
        fclose(table_file);
    }
    return (oJSON);
}

/************************************************************************/
/*                         cmor_dataset_json()                          */
/************************************************************************/
int cmor_dataset_json(char *ressource)
{
    extern cmor_dataset_def cmor_current_dataset;

    int ierr;
    cmor_add_traceback("cmor_dataset_json");
    cmor_is_setup();

    char szVal[CMOR_MAX_STRING];
    json_object *json_obj;

    strncpytrim(cmor_current_dataset.path_template,
                CMOR_DEFAULT_PATH_TEMPLATE, CMOR_MAX_STRING);

    strncpytrim(cmor_current_dataset.file_template,
                CMOR_DEFAULT_FILE_TEMPLATE, CMOR_MAX_STRING);

    strncpytrim(cmor_current_dataset.furtherinfourl,
                "", CMOR_MAX_STRING);

    strncpytrim(cmor_current_dataset.history_template,
                CMOR_DEFAULT_HISTORY_TEMPLATE, CMOR_MAX_STRING);

    json_obj = cmor_open_inpathFile(ressource);
    if (json_obj == NULL) {
        return (1);
    }

/* -------------------------------------------------------------------- */
/* Set Default values for CV, AXIS_ENTRY and FORMULA VAR Filename       */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(CMOR_INPUTFILENAME, ressource, 1);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_CV_FILENAME,
                                            TABLE_CONTROL_FILENAME, 1);
    cmor_set_cur_dataset_attribute_internal(CMOR_AXIS_ENTRY_FILE,
                                            AXIS_ENTRY_FILENAME, 1);
    cmor_set_cur_dataset_attribute_internal(CMOR_FORMULA_VAR_FILE,
                                            FORMULA_VAR_FILENAME, 1);

/* -------------------------------------------------------------------- */
/* Set all user defined input.  Default value above can be superseded   */
/* by user input.                                                       */
/* -------------------------------------------------------------------- */

    json_object_object_foreach(json_obj, key, value) {
        if (!value) {
            continue;
        }
        if (key[0] == '#') {
            continue;
        }
        strcpy(szVal, json_object_get_string(value));

        if (strcmp(key, FILE_OUTPUTPATH) == 0) {
            strncpytrim(cmor_current_dataset.outpath, szVal, CMOR_MAX_STRING);
            continue;
        } else if (strcmp(key, FILE_PATH_TEMPLATE) == 0) {
            strncpytrim(cmor_current_dataset.path_template,
                        szVal, CMOR_MAX_STRING);
            continue;
        } else if (strcmp(key, FILE_NAME_TEMPLATE) == 0) {
            strncpytrim(cmor_current_dataset.file_template,
                        szVal, CMOR_MAX_STRING);
            continue;
        } else if (strcmp(key, GLOBAL_ATT_HISTORYTMPL) == 0) {
            strncpytrim(cmor_current_dataset.history_template,
                        szVal, CMOR_MAX_STRING);
            continue;

        } else if (strcmp(key, GLOBAL_ATT_FURTHERINFOURL) == 0) {
            strncpytrim(cmor_current_dataset.furtherinfourl,
                        szVal, CMOR_MAX_STRING);
            continue;
        }
        cmor_set_cur_dataset_attribute_internal(key, szVal, 1);
    }

    cmor_current_dataset.initiated = 1;
    cmor_generate_uuid();
/* -------------------------------------------------------------------- */
/*  Verify if the output directory exist                                */
/* -------------------------------------------------------------------- */
    ierr = cmor_outpath_exist(cmor_current_dataset.outpath);
    if (ierr) {
        cmor_pop_traceback();
        return (1);
    }
    if (json_obj != NULL) {
        json_object_put(json_obj);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                     cmor_put_nc_num_attribute()                      */
/************************************************************************/
int cmor_put_nc_num_attribute(int ncid, int nc_var_id, char *name, char type,
                              double value, char *var_name)
{
    int ierr;

    cmor_add_traceback("cmor_put_nc_num_attribute");
    ierr = 0;
    if (type == 'i') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_INT, 1, &value);
    } else if (type == 'l') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_INT, 1, &value);
    } else if (type == 'f') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_FLOAT, 1, &value);
    } else if (type == 'd') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_DOUBLE, 1, &value);
    }
    if (ierr != NC_NOERR) {
        cmor_handle_error_variadic(
            "NetCDF Error (%i: %s) setting numerical attribute"
            " %s on variable %s",
            CMOR_CRITICAL,
            ierr, nc_strerror(ierr), name, var_name);
    }
    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                     cmor_put_nc_char_attribute()                     */
/************************************************************************/
int cmor_put_nc_char_attribute(int ncid,
                               int nc_var_id,
                               char *name, char *value, char *var_name)
{
    int k, ierr;

    ierr = 0;
    cmor_add_traceback("cmor_put_nc_char_attribute");
    k = strlen(value);
    if (k != 0) {
        value[k] = '\0';
        ierr = nc_put_att_text(ncid, nc_var_id, name, k + 1, value);
        if (ierr != NC_NOERR) {
            cmor_handle_error_variadic(
                "NetCDF Error (%i: %s) setting attribute: '%s' "
                "on variable (%s)",
                CMOR_CRITICAL,
                ierr, nc_strerror(ierr), name, var_name);
        }
    }
    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                  cmor_set_cur_dataset_attribute()                    */
/************************************************************************/
int cmor_set_cur_dataset_attribute(char *name, char *value, int optional)
{
    int rc;
    cmor_add_traceback("cmor_set_cur_dataset_attribute");
    cmor_is_setup();

    rc = cmor_set_cur_dataset_attribute_internal(name, value, optional);

    cmor_pop_traceback();
    return (rc);
}

/************************************************************************/
/*              cmor_set_cur_dataset_attribute_internal()               */
/************************************************************************/
int cmor_set_cur_dataset_attribute_internal(char *name, char *value,
                                            int optional)
{
    int i, n;
    char msg[CMOR_MAX_STRING];
    extern cmor_dataset_def cmor_current_dataset;

    cmor_add_traceback("cmor_set_cur_dataset_attribute_internal");
    cmor_is_setup();

    cmor_trim_string(value, msg);

    if ((int)strlen(name) > CMOR_MAX_STRING) {
        cmor_handle_error_variadic(
            "Dataset error, attribute name: %s; length (%i) is "
            "greater than limit: %i",
            CMOR_NORMAL,
            name, (int)strlen(name), CMOR_MAX_STRING);
        cmor_pop_traceback();
        return (1);
    }


//    // clear attribute
    if ((strcmp(value,"") == 0) && (optional == 0)){
        for (i = 0; i <= cmor_current_dataset.nattributes; i++) {
            if (strcmp(name, cmor_current_dataset.attributes[i].names) == 0) {
                n = i;
                break;
            }
        }
        if (i != cmor_current_dataset.nattributes - 1) {
            strcpy(cmor_current_dataset.attributes[i].values, "");
            cmor_pop_traceback();
            return (0);
        }
    }

    if ((value == NULL) || (msg[0] == '\0')) {
        if (optional == 1) {
            cmor_pop_traceback();
            return (0);
        } else {
            cmor_handle_error_variadic(
                "Dataset error, required attribute %s was not "
                "passed or blanked", CMOR_CRITICAL, name);
            cmor_pop_traceback();
            return (1);
        }
    }


    cmor_trim_string(name, msg);
    n = cmor_current_dataset.nattributes;

    for (i = 0; i <= cmor_current_dataset.nattributes; i++) {
        if (strcmp(msg, cmor_current_dataset.attributes[i].names) == 0) {
            n = i;
            cmor_current_dataset.nattributes -= 1;
            break;
        }
    }

    if (n >= CMOR_MAX_ATTRIBUTES) {
        cmor_handle_error_variadic(
            "Setting dataset attribute: %s, we already have %i "
            "elements set which is the max, this element won't be set",
            CMOR_NORMAL,
            name, CMOR_MAX_ELEMENTS);
        cmor_pop_traceback();
        return (1);
    }

    if (strcmp(msg, FILE_PATH_TEMPLATE) == 0) {
        cmor_trim_string(value, msg);
        strncpytrim(cmor_current_dataset.path_template, msg, CMOR_MAX_STRING);
    } else if (strcmp(msg, FILE_NAME_TEMPLATE) == 0) {
        cmor_trim_string(value, msg);
        strncpytrim(cmor_current_dataset.file_template, msg, CMOR_MAX_STRING);

    } else if (strcmp(msg, GLOBAL_ATT_FURTHERINFOURLTMPL) == 0) {
        cmor_trim_string(value, msg);
        strncpytrim(cmor_current_dataset.furtherinfourl, msg, CMOR_MAX_STRING);
    } else if (strcmp(msg, GLOBAL_ATT_HISTORYTMPL) == 0) {
        cmor_trim_string(value, msg);
        strncpytrim(cmor_current_dataset.history_template, msg, CMOR_MAX_STRING);

    } else {
        strncpy(cmor_current_dataset.attributes[n].names, msg, CMOR_MAX_STRING);
        cmor_trim_string(value, msg);
        strncpytrim(cmor_current_dataset.attributes[n].values, msg,
                    CMOR_MAX_STRING);
        cmor_current_dataset.nattributes += 1;
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                   cmor_get_cur_dataset_attribute()                   */
/************************************************************************/
int cmor_get_cur_dataset_attribute(char *name, char *value)
{
    int i, n;
    extern cmor_dataset_def cmor_current_dataset;

    cmor_add_traceback("cmor_get_cur_dataset_attribute");
    cmor_is_setup();
    if (strlen(name) > CMOR_MAX_STRING) {
        cmor_handle_error_variadic(
            "Dataset: %s length is greater than limit: %i",
            CMOR_NORMAL, name, CMOR_MAX_STRING);
        cmor_pop_traceback();
        return (1);
    }
    n = -1;
    for (i = 0; i <= cmor_current_dataset.nattributes; i++) {
        if (strcmp(name, cmor_current_dataset.attributes[i].names) == 0)
            n = i;
    }
    if (n == -1) {
        cmor_handle_error_variadic(
            "Dataset: current dataset does not have attribute : %s",
            CMOR_NORMAL, name);
        cmor_pop_traceback();
        return (1);
    }
    strncpy(value, cmor_current_dataset.attributes[n].values, CMOR_MAX_STRING);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                   cmor_has_cur_dataset_attribute()                   */
/************************************************************************/
int cmor_has_cur_dataset_attribute(char *name)
{
    int i, n;
    extern cmor_dataset_def cmor_current_dataset;

    cmor_add_traceback("cmor_has_cur_dataset_attribute");
    cmor_is_setup();
    if ((int)strlen(name) > CMOR_MAX_STRING) {
        cmor_handle_error_variadic(
            "Dataset: attribute name (%s) length\n! "
            "(%i) is greater than limit: %i",
            CMOR_NORMAL,
            name, (int)strlen(name), CMOR_MAX_STRING);
        cmor_pop_traceback();
        return (1);
    }
    n = -1;
    for (i = 0; i <= cmor_current_dataset.nattributes; i++) {
        if (strcmp(name, cmor_current_dataset.attributes[i].names) == 0)
            n = i;
    }
    if (n == -1) {
        cmor_pop_traceback();
        return (1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                      cmor_outpath_exist()                            */
/************************************************************************/
int cmor_outpath_exist(char *outpath)
{
    struct stat buf;
    char msg[CMOR_MAX_STRING];
    FILE *test_file = NULL;
    int pid;
    int ierr;

    cmor_add_traceback("cmor_outpath_exist");
    cmor_is_setup();

/* -------------------------------------------------------------------- */
/*      Very first thing is to make sure the output path does exist     */
/* -------------------------------------------------------------------- */
    if (stat(cmor_current_dataset.outpath, &buf) == 0) {
        if (S_ISREG(buf.st_mode) != 0) {
            cmor_handle_error_variadic(
                "You defined your output directory to be: '%s',\n! "
                "but it appears to be a regular file not a directory",
                CMOR_CRITICAL,
                cmor_current_dataset.outpath);
            cmor_pop_traceback();
            return (1);
        } else if (S_ISDIR(buf.st_mode) == 0) {
            cmor_handle_error_variadic(
                "You defined your output directory to be: '%s',\n! "
                "but it appears to be a special file not a directory",
                CMOR_CRITICAL,
                cmor_current_dataset.outpath);
            cmor_pop_traceback();
            return (1);

        }
/* -------------------------------------------------------------------- */
/*      ok if not root then test permissions                            */
/* -------------------------------------------------------------------- */
        if (getuid() != 0) {
            pid = getpid();
            sprintf(msg,"%s/tmp%i.cmor.test", 
                    cmor_current_dataset.outpath, pid);
            test_file = fopen(msg, "w");
            if (test_file == NULL) {

                cmor_handle_error_variadic(
                    "You defined your output directory to be: '%s', but\n! "
                    "you do not have read/write permissions on it",
                    CMOR_CRITICAL,
                    cmor_current_dataset.outpath);
                cmor_pop_traceback();
                return (1);

            } else {
                fclose(test_file);
                remove(msg);
            }
        }
    } else if (errno == ENOENT) {
        cmor_handle_error_variadic(
            "You defined your output directory to be: '%s', but this\n! "
            "directory does not exist. CMOR will create it!",
            CMOR_WARNING,
            cmor_current_dataset.outpath);
/* -------------------------------------------------------------------- */
/* Create directory with 755 permission for user                        */
/* -------------------------------------------------------------------- */
        ierr = mkdir(cmor_current_dataset.outpath, S_IRWXU | S_IRGRP | S_IXGRP |
                     S_IROTH | S_IXOTH);
        if (ierr != 0) {
            cmor_handle_error_variadic(
                "CMOR was unable to create this directory %s\n! "
                "You do not have write permissions!",
                CMOR_CRITICAL,
                cmor_current_dataset.outpath);
            cmor_pop_traceback();
            return (1);
        }

    } else if (errno == EACCES) {
        cmor_handle_error_variadic(
            "You defined your output directory to be: '%s', but we\n! "
            "cannot access it, please check permissions",
            CMOR_CRITICAL,
            cmor_current_dataset.outpath);
        cmor_pop_traceback();
        return (1);

    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_convert_string_to_list()                     */
/************************************************************************/
int cmor_convert_string_to_list(char *invalues, char type, void **target,
                                int *nelts)
{
    int i, j, k, itmp;
    long l;
    double d;
    float f;
    char values[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];

    j = 1;
    cmor_add_traceback("cmor_convert_string_to_list");

/* -------------------------------------------------------------------- */
/*      trim this so no extra spaces after or before                    */
/* -------------------------------------------------------------------- */
    strncpytrim(values, invalues, CMOR_MAX_STRING);
    k = 1;                      /* 1 means we are on characters */
    for (i = 0; i < strlen(values); i++) {
        if (values[i] == ' ') {
            if (k == 1) {
                j++;
                k = 0;
            }
            while (values[i + 1] == ' ')
                i++;
        } else {
            k = 1;
        }
    }

    *nelts = j;

    if (type == 'i')
        *target = malloc(j * sizeof(int));
    else if (type == 'f')
        *target = malloc(j * sizeof(float));
    else if (type == 'l')
        *target = malloc(j * sizeof(long));
    else if (type == 'd')
        *target = malloc(j * sizeof(double));
    else if (type == 'c')
        *target = (char **)malloc(j * sizeof(char *));
    else {
        cmor_handle_error_variadic(
            "unknown conversion '%c' for list: %s",
            CMOR_CRITICAL, type, values);
    }

    if (*target == NULL) {
        cmor_handle_error_variadic(
            "mallocing '%c' for list: %s",
            CMOR_CRITICAL, type, values);
    }

    j = 0;
    msg[0] = '\0';
    k = 0;
    itmp = 1;
    for (i = 0; i < strlen(values); i++) {
        if (values[i] == ' ') { /* ok next world */
            if (itmp == 1) {
                itmp = 0;
                msg[i - k] = '\0';
                strncpytrim(msg2, msg, CMOR_MAX_STRING);
                if (type == 'i') {
                    sscanf(msg2, "%d", &itmp);
                    ((int *)*target)[j] = (int)itmp;
                } else if (type == 'l') {
                    sscanf(msg2, "%ld", &l);
                    ((long *)*target)[j] = (long)l;
                } else if (type == 'f') {
                    sscanf(msg2, "%f", &f);
                    ((float *)*target)[j] = (float)f;
                } else if (type == 'd') {
                    sscanf(msg2, "%lf", &d);
                    ((double *)*target)[j] = (double)d;
                } else if (type == 'c') {
                    ((char **)*target)[j] = (char *)malloc(13 * sizeof(char));
                    strncpy(((char **)*target)[j], msg2, 12);
                }
                j++;
            }
            while (values[i + 1] == ' ')
                i++;
            k = i + 1;
        } else {
            msg[i - k] = values[i];
            itmp = 1;
        }
    }
/* -------------------------------------------------------------------- */
/*      ok now the last one                                             */
/* -------------------------------------------------------------------- */
    msg[i - k] = '\0';

    strncpytrim(msg2, msg, CMOR_MAX_STRING);
    if (type == 'i') {
        sscanf(msg2, "%d", &itmp);
        ((int *)*target)[j] = (int)itmp;
    } else if (type == 'l') {
        sscanf(msg2, "%ld", &l);
        ((long *)*target)[j] = (long)l;
    } else if (type == 'f') {
        sscanf(msg2, "%f", &f);
        ((float *)*target)[j] = (float)f;
    } else if (type == 'd') {
        sscanf(msg2, "%lf", &d);
        ((double *)*target)[j] = (double)d;
    } else if (type == 'c') {
        ((char **)*target)[j] = (char *)malloc(13 * sizeof(char));
        strncpy(((char **)*target)[j], msg2, 12);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                     cmor_define_zfactors_vars()                      */
/************************************************************************/
int cmor_define_zfactors_vars(int var_id, int ncid, int *nc_dim,
                              char *formula_terms, int *nzfactors,
                              int *zfactors, int *nc_zfactors, int i,
                              int dim_bnds)
{
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    int ierr = 0, l, m, k, n, j, m2, found, nelts, *int_list = NULL;
    int dim_holder[CMOR_MAX_VARIABLES];
    int lnzfactors;
    int ics, icd, icdl, icz, ia;
    cmor_add_traceback("cmor_define_zfactors_vars");
    cmor_is_setup();
    lnzfactors = *nzfactors;

/* -------------------------------------------------------------------- */
/*      now figures out the variables for z_factor and loops thru it    */
/* -------------------------------------------------------------------- */
    n = strlen(formula_terms);
    for (j = 0; j < n; j++) {
        while ((formula_terms[j] != ':') && (j < n)) {
            j++;
        }
/* -------------------------------------------------------------------- */
/*      at this point we skipped the name thingy                        */
/* -------------------------------------------------------------------- */
        j++;
        while (formula_terms[j] == ' ') {
            j++;
        }                       /* ok we skipped the blanks as well */
        /* ok now we can start scanning the zvar name */
        k = j;
        while ((formula_terms[j] != ' ') && (formula_terms[j] != '\0')) {
            ctmp[j - k] = formula_terms[j];
            j++;
        }

/* -------------------------------------------------------------------- */
/*      all right here we reach a  blank, the name is finsihed          */
/* -------------------------------------------------------------------- */
        ctmp[j - k] = '\0';

/* -------------------------------------------------------------------- */
/*      here we try to match with the actual variable                   */
/* -------------------------------------------------------------------- */
        l = -1;
        for (k = 0; k < cmor_nvars + 1; k++) {
            if (strcmp(ctmp, cmor_vars[k].id) == 0) {
/* -------------------------------------------------------------------- */
/*      ok that is not enough! We need to know if the dims match!       */
/* -------------------------------------------------------------------- */
                nelts = 0;
                for (m = 0; m < cmor_vars[k].ndims; m++) {
                    for (m2 = 0; m2 < cmor_vars[var_id].ndims; m2++) {
                        if (cmor_vars[k].axes_ids[m]
                            == cmor_vars[var_id].axes_ids[m2]) {
                            nelts += 1;
                            break;
                        }
                    }
                }
                if (nelts == cmor_vars[k].ndims) {
                    l = k;
                    break;
                }
            }
        }

        if (l == -1) {
/* -------------------------------------------------------------------- */
/*      ok this looks bad! last hope is that the                        */
/*      zfactor is actually a coordinate!                               */
/* -------------------------------------------------------------------- */
            found = 0;
            for (m = 0; m < cmor_vars[var_id].ndims; m++) {
                if (strcmp(cmor_axes[cmor_vars[var_id].axes_ids[m]].id, ctmp)
                    == 0) {
                    found = 1;
                    break;
                }
/* -------------------------------------------------------------------- */
/*      ok this axes has bounds let's check against his name + _bnds then*/
/* -------------------------------------------------------------------- */
                if (cmor_axes[cmor_vars[var_id].axes_ids[m]].bounds != NULL) {

                    sprintf(msg, "%s_bnds",
                            cmor_axes[cmor_vars[var_id].axes_ids[m]].id);
                    if (strcmp(msg, ctmp) == 0) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 0) {
                cmor_handle_error_var_variadic(
                    "could not find the zfactor variable: %s. \n! "
                    "Please define zfactor before defining the\n! "
                    "variable %s (table %s).\n! \n! "
                    "Also zfactor dimensions must match variable's"
                    " dimensions.\n! ",
                    CMOR_CRITICAL, var_id,
                    ctmp,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
                cmor_pop_traceback();
                return (1);
            }
        } else {
            found = 0;
        }
/* -------------------------------------------------------------------- */
/*      now figure out if we already defined this zfactor var           */
/* -------------------------------------------------------------------- */

        for (k = 0; k < lnzfactors; k++)
            if (zfactors[k] == l)
                found = 1;
        if (found == 0) {
/* -------------------------------------------------------------------- */
/*      ok it is a new one                                              */
/* -------------------------------------------------------------------- */

            zfactors[lnzfactors] = l;
/* -------------------------------------------------------------------- */
/*      ok we need to figure out the dimensions of that zfactor         */
/*      and then define the variable                                    */
/* -------------------------------------------------------------------- */
            for (k = 0; k < cmor_vars[l].ndims; k++) {
                found = 0;
                for (m = 0; m < cmor_vars[var_id].ndims; m++) {
                    if (strcmp(cmor_axes[cmor_vars[var_id].axes_ids[m]].id,
                               cmor_axes[cmor_vars[l].axes_ids[k]].id) == 0) {
                        found = 1;
                        dim_holder[k] = nc_dim[m];
/* -------------------------------------------------------------------- */
/*      ok here we mark this factor has time varying if necessary       */
/*      so that we can count the number of time written and make        */
/*      sure it matches the variable                                    */
/* -------------------------------------------------------------------- */
                        if (cmor_axes[cmor_vars[var_id].axes_ids[m]].axis
                            == 'T') {
                            for (ia = 0; ia < 10; ia++) {
                                if (cmor_vars[var_id].associated_ids[ia]
                                    == -1) {
                                    cmor_vars[var_id].associated_ids[ia] = l;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                if (found == 0) {
                    cmor_handle_error_var_variadic(
                        "variable \"%s\" (table: %s) has axis \"%s\"\n! "
                        "defined with formula terms, but term \"%s\"\n! "
                        "depends on axis \"%s\" which is not part of\n! "
                        "the variable",
                        CMOR_CRITICAL, var_id,
                        cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].id, ctmp,
                        cmor_axes[cmor_vars[l].axes_ids[k]].id);
                }
            }
/* -------------------------------------------------------------------- */
/*      at that point we can define the var                             */
/* -------------------------------------------------------------------- */
            if (dim_bnds == -1) {       /* we are not defining a bnds one */
                if (cmor_vars[l].type == 'd')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_DOUBLE,
                                      cmor_vars[l].ndims, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                else if (cmor_vars[l].type == 'f')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_FLOAT,
                                      cmor_vars[l].ndims, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                else if (cmor_vars[l].type == 'l')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_INT,
                                      cmor_vars[l].ndims, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                else if (cmor_vars[l].type == 'i')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_INT,
                                      cmor_vars[l].ndims, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NC Error (%i: %s) for variable %s (table %s)\n! "
                        "error defining zfactor var: %i (%s)",
                        CMOR_CRITICAL, var_id,
                        ierr,
                        nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id, lnzfactors, cmor_vars[l].id);
                }

/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
                if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                    && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                    && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
                    if (cmor_vars[l].ndims > 0) {
                        int nTableID = cmor_vars[l].ref_table_id;
                        ics = cmor_tables[nTableID].vars[nTableID].shuffle;
                        icd = cmor_tables[nTableID].vars[nTableID].deflate;
                        icdl =
                          cmor_tables[nTableID].vars[nTableID].deflate_level;
                        icz =
                          cmor_tables[nTableID].vars[nTableID].zstandard_level;

                        if (icd != 0) {
                            ierr |= nc_def_var_deflate(ncid, nc_zfactors[lnzfactors],
                                                    ics, icd, icdl);
                        } else {
                            ierr |= nc_def_var_deflate(ncid, nc_zfactors[lnzfactors],
                                                    ics, 0, 0);
                            ierr |= nc_def_var_zstandard(ncid, nc_zfactors[lnzfactors],
                                                    icz);
                        }

                        if (ierr != NC_NOERR) {
                            cmor_handle_error_var_variadic(
                                "NCError (%i: %s) defining compression\n! "
                                "parameters for zfactor variable %s for\n! "
                                "variable '%s' (table %s)",
                                CMOR_CRITICAL, var_id,
                                ierr,
                                nc_strerror(ierr), cmor_vars[l].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nTableID].szTable_id);
                        }
                    }
                }

/* -------------------------------------------------------------------- */
/*      Creates attribute related to that variable                      */
/* -------------------------------------------------------------------- */
                for (k = 0; k < cmor_vars[l].nattributes; k++) {
/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
                    if (cmor_has_variable_attribute(l,
                                                    cmor_vars[l].attributes[k])
                        != 0) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
                        continue;
                    }
                    if (strcmp(cmor_vars[l].attributes[k], "flag_values")
                        == 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */
                        ierr =
                          cmor_convert_string_to_list(cmor_vars
                                                      [l].attributes_values_char
                                                      [k], 'i',
                                                      (void *)&int_list,
                                                      &nelts);

                        ierr = nc_put_att_int(ncid, nc_zfactors[lnzfactors],
                                              "flag_values",
                                              NC_INT, nelts, int_list);

                        if (ierr != NC_NOERR) {
                            cmor_handle_error_var_variadic(
                                "NetCDF Error (%i: %s) setting flags\n! "
                                "numerical attribute on zfactor\n! "
                                "variable %s for variable %s (table %s)",
                                CMOR_CRITICAL, var_id,
                                ierr, nc_strerror(ierr), cmor_vars[l].id,
                                cmor_vars[var_id].id,
                                cmor_tables[cmor_vars
                                            [var_id].
                                            ref_table_id].szTable_id);
                        }
                        free(int_list);
                    } else if (cmor_vars[l].attributes_type[k] == 'c') {
                        ierr = cmor_put_nc_char_attribute(ncid,
                                                          nc_zfactors
                                                          [lnzfactors],
                                                          cmor_vars
                                                          [l].attributes[k],
                                                          cmor_vars
                                                          [l].
                                                          attributes_values_char
                                                          [k], cmor_vars[l].id);
                    } else {
                        ierr = cmor_put_nc_num_attribute(ncid,
                                                         nc_zfactors
                                                         [lnzfactors],
                                                         cmor_vars[l].attributes
                                                         [k],
                                                         cmor_vars
                                                         [l].attributes_type[k],
                                                         cmor_vars
                                                         [l].
                                                         attributes_values_num
                                                         [k], cmor_vars[l].id);
                    }
                }
                lnzfactors += 1;
            } else {
/* -------------------------------------------------------------------- */
/*      ok now we need to see if we have bounds on that variable        */
/* -------------------------------------------------------------------- */
                dim_holder[cmor_vars[l].ndims] = dim_bnds;
                if (cmor_vars[l].type == 'd')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_DOUBLE,
                                      cmor_vars[l].ndims + 1, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                else if (cmor_vars[l].type == 'f')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_FLOAT,
                                      cmor_vars[l].ndims + 1, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                else if (cmor_vars[l].type == 'l')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_INT,
                                      cmor_vars[l].ndims + 1, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                else if (cmor_vars[l].type == 'i')
                    ierr = nc_def_var(ncid, cmor_vars[l].id, NC_INT,
                                      cmor_vars[l].ndims + 1, &dim_holder[0],
                                      &nc_zfactors[lnzfactors]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NC Error (%i: %s) for variable %s (table: %s),\n! "
                        "error defining zfactor var: %i (%s)",
                        CMOR_CRITICAL, var_id,
                        ierr,
                        nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id, lnzfactors, cmor_vars[l].id);
                }

/* -------------------------------------------------------------------- */
/*      Creates attribute related to that variable                      */
/* -------------------------------------------------------------------- */
                for (k = 0; k < cmor_vars[l].nattributes; k++) {
/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
                    if (cmor_has_variable_attribute(l,
                                                    cmor_vars[l].attributes[k])
                        != 0) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
                        continue;
                    }
                    if (strcmp(cmor_vars[l].attributes[k], "flag_values")
                        == 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */
                        ierr =
                          cmor_convert_string_to_list(cmor_vars
                                                      [l].attributes_values_char
                                                      [k], 'i',
                                                      (void *)&int_list,
                                                      &nelts);

                        ierr = nc_put_att_int(ncid, nc_zfactors[lnzfactors],
                                              "flag_values",
                                              NC_INT, nelts, int_list);

                        if (ierr != NC_NOERR) {
                            cmor_handle_error_var_variadic(
                                "NetCDF Error (%i: %s) setting flags numerical "
                                "attribute on zfactor variable %s for variable "
                                "%s (table: %s)",
                                CMOR_CRITICAL, var_id,
                                ierr,
                                nc_strerror(ierr), cmor_vars[l].id,
                                cmor_vars[var_id].id,
                                cmor_tables[cmor_vars
                                            [var_id].
                                            ref_table_id].szTable_id);
                        }
                        free(int_list);

                    } else if (cmor_vars[l].attributes_type[k] == 'c') {
                        ierr = cmor_put_nc_char_attribute(ncid,
                                                          nc_zfactors
                                                          [lnzfactors],
                                                          cmor_vars
                                                          [l].attributes[k],
                                                          cmor_vars
                                                          [l].
                                                          attributes_values_char
                                                          [k], cmor_vars[l].id);
                    } else {
                        ierr = cmor_put_nc_num_attribute(ncid,
                                                         nc_zfactors
                                                         [lnzfactors],
                                                         cmor_vars[l].attributes
                                                         [k],
                                                         cmor_vars
                                                         [l].attributes_type[k],
                                                         cmor_vars
                                                         [l].
                                                         attributes_values_num
                                                         [k], cmor_vars[l].id);
                    }
                }

                lnzfactors += 1;
            }
        }

        while ((formula_terms[j] == ' ') && (formula_terms[j] != '\0')) {
            j++;
        }                       /* skip the other whites */
    }
    *nzfactors = lnzfactors;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                          cmor_flip_hybrid()                          */
/************************************************************************/
void cmor_flip_hybrid(int var_id, int i, char *a, char *b, char *abnds,
                      char *bbnds)
{
    int doflip, j, k, l = 0;
    double tmp;
    extern cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
    extern cmor_axis_t cmor_axes[CMOR_MAX_AXES];

    cmor_add_traceback("cmor_flip_hybrid");

/* -------------------------------------------------------------------- */
/*      here we need to look and see if we need to flip the             */
/*      levels again since we overwrote this stuff                      */
/* -------------------------------------------------------------------- */
    doflip = 0;
    cmor_axis_t *pVarAxis;
    cmor_axis_def_t *pTableAxis;
    int nVarAxisTableID;

    pVarAxis = &cmor_axes[cmor_vars[var_id].axes_ids[i]];
    nVarAxisTableID = pVarAxis->ref_table_id;

    pTableAxis = &cmor_tables[nVarAxisTableID].axes[nVarAxisTableID];

    if (pTableAxis->stored_direction == 'd') {
/* -------------------------------------------------------------------- */
/*      decrease stuff                                                  */
/* -------------------------------------------------------------------- */
        if (pVarAxis->values[1] > pVarAxis->values[0])
            doflip = 1;
    } else {
/* -------------------------------------------------------------------- */
/*      increase stuff                                                  */
/* -------------------------------------------------------------------- */
        if (pVarAxis->values[1] < pVarAxis->values[0])
            doflip = 1;
    }

    if (doflip == 1) {
/* -------------------------------------------------------------------- */
/*      look for a coeff                                                */
/* -------------------------------------------------------------------- */
        k = -1;
        for (j = 0; j <= cmor_nvars; j++)
            if ((strcmp(cmor_vars[j].id, a) == 0) &&
                (cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i])) {
                k = j;
                break;
            }
/* -------------------------------------------------------------------- */
/*      look for b coeff                                                */
/* -------------------------------------------------------------------- */
        if (b != NULL) {
            l = -1;
            for (j = 0; j <= cmor_nvars; j++) {
                if ((strcmp(cmor_vars[j].id, b) == 0) &&
                    (cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i])) {
                    l = j;
                    break;
                }
            }
        }

        for (j = 0; j < pVarAxis->length / 2; j++) {

            tmp = pVarAxis->values[j];
            pVarAxis->values[j] = pVarAxis->values[pVarAxis->length - 1 - j];
            pVarAxis->values[pVarAxis->length - 1 - j] = tmp;

            tmp = cmor_vars[k].values[j];
            cmor_vars[k].values[j] =
              cmor_vars[k].values[pVarAxis->length - 1 - j];
            cmor_vars[k].values[pVarAxis->length - 1 - j] = tmp;

            if (b != NULL) {
                tmp = cmor_vars[l].values[j];
                cmor_vars[l].values[j] =
                  cmor_vars[l].values[pVarAxis->length - 1 - j];
                cmor_vars[l].values[pVarAxis->length - 1 - j] = tmp;
            }
        }

        if (pVarAxis->bounds != NULL) {
            k = -1;
            for (j = 0; j <= cmor_nvars; j++) {
                if ((strcmp(cmor_vars[j].id, abnds) == 0) &&
                    (cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i])) {
                    k = j;
                    break;
                }
            }

            if (bbnds != NULL) {
                l = -1;
                for (j = 0; j <= cmor_nvars; j++) {
                    if ((strcmp(cmor_vars[j].id, bbnds) == 0) &&
                        (cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i])) {
                        l = j;
                        break;
                    }
                }
            }

            for (j = 0; j < pVarAxis->length; j++) {
                int length = pVarAxis->length;
                tmp = pVarAxis->bounds[j];
                pVarAxis->bounds[j] = pVarAxis->bounds[length * 2 - 1 - j];
                pVarAxis->bounds[length * 2 - 1 - j] = tmp;

                tmp = cmor_vars[k].values[j];
                cmor_vars[k].values[j] =
                  cmor_vars[k].values[length * 2 - 1 - j];
                cmor_vars[k].values[length * 2 - 1 - j] = tmp;

                if (bbnds != NULL) {
                    tmp = cmor_vars[l].values[j];
                    cmor_vars[l].values[j] =
                      cmor_vars[l].values[length * 2 - 1 - j];
                    cmor_vars[l].values[length * 2 - 1 - j] = tmp;
                }
            }
        }
    }
    cmor_pop_traceback();
    return;
}

/************************************************************************/
/*                          cmor_set_refvar( )                          */
/************************************************************************/
int cmor_set_refvar(int var_id, int *refvar, int ntimes_passed)
{

/* -------------------------------------------------------------------- */
/*  Return either associated variable id or passed variable id          */
/* -------------------------------------------------------------------- */
    int nRefVarID = var_id;
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int ierr;

    cmor_add_traceback("cmor_set_refvar");
    if (refvar != NULL) {
        nRefVarID = (int)*refvar;

        if (cmor_vars[nRefVarID].initialized == -1) {
            cmor_handle_error_var_variadic(
                "You are trying to write variable \"%s\" in association\n! "
                "with variable \"%s\" (table %s), but you you need to\n! "
                "write the associated variable first in order to\n! "
                "initialize the file and dimensions.",
                CMOR_CRITICAL, var_id,
                cmor_vars[nRefVarID].id,
                cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id);
        }
/* -------------------------------------------------------------------- */
/*      ok now we need to scan the netcdf file                          */
/*      to figure the ncvarid associated                                */
/* -------------------------------------------------------------------- */
        ierr = nc_inq_varid(cmor_vars[nRefVarID].initialized,
                            cmor_vars[var_id].id, &cmor_vars[var_id].nc_var_id);

        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "Could not find variable: '%s' (table: %s) in file of\n! "
                "associated variable: '%s'",
                CMOR_CRITICAL, var_id,
                cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id,
                cmor_vars[*refvar].id);
        }
        cmor_vars[var_id].ntimes_written =
          cmor_vars[nRefVarID].ntimes_written - ntimes_passed;
    }
    cmor_pop_traceback();
    return (nRefVarID);
}

/************************************************************************/
/*                    cmor_validate_activity_id()                       */
/************************************************************************/
int cmor_validate_activity_id(int nVarRefTblID)
{
    int ierr = 0;

    cmor_add_traceback("cmor_validate_activity_id");

    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                       cmor_checkMissing()                            */
/************************************************************************/
void cmor_checkMissing(int varid, int var_id, char type)
{
    int nVarRefTblID;

    cmor_add_traceback("cmor_checkMissing");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

    if (cmor_vars[varid].nomissing == 0) {
        if (cmor_vars[varid].itype != type) {
            cmor_handle_error_variadic(
                "You defined variable \"%s\" (table %s) with a missing\n! "
                "value of type \"%c\", but you are now writing data of\n! "
                "type: \"%c\" this may lead to some spurious handling\n! "
                "of the missing values",
                CMOR_WARNING,
                cmor_vars[varid].id,
                cmor_tables[nVarRefTblID].szTable_id,
                cmor_vars[varid].itype, type);
        }
    }
    cmor_pop_traceback();
}
/************************************************************************/
/*                           copyfile()                                 */
/************************************************************************/
int copyfile(const char *to, const char *from) {
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return (-1);

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        unlink(from);

        return (0);
    }

    out_error: saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return (-1);
}

/************************************************************************/
/*                    cmor_validateFilename()                           */
/************************************************************************/
int cmor_validateFilename(char *outname, char *file_suffix, int var_id)
{
    int cmode;
    int ierr;
    FILE *fperr;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    size_t starts[2];
    size_t nctmp;
    int ncid;
    int i;
    cmor_add_traceback("cmor_validateFilename");
    ncid = -1;
    ierr = 0;
    if (USE_NETCDF_4 == 1) {
        cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
        if ((CMOR_NETCDF_MODE == CMOR_REPLACE_3)
            || (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)
            || (CMOR_NETCDF_MODE == CMOR_APPEND_3)) {
            cmode = NC_CLOBBER;
        }
    } else {
        cmode = NC_CLOBBER;
    }

    if ((CMOR_NETCDF_MODE == CMOR_REPLACE_4)
        || (CMOR_NETCDF_MODE == CMOR_REPLACE_3)) {

        ierr = nc_create(outname, NC_CLOBBER | cmode, &ncid);

    } else if ((CMOR_NETCDF_MODE == CMOR_PRESERVE_4)
               || (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)) {
/* -------------------------------------------------------------------- */
/*      ok first let's check if the file does exists or not             */
/* -------------------------------------------------------------------- */
        fperr = NULL;
        fperr = fopen(outname, "r");
        if (fperr != NULL) {
            cmor_handle_error_var_variadic(
                "Output file ( %s ) already exists, remove file\n! "
                "or use CMOR_REPLACE or CMOR_APPEND for\n! "
                "CMOR_NETCDF_MODE value in cmor_setup",
                CMOR_CRITICAL, var_id, outname);
            ierr = fclose(fperr);
            fperr = NULL;
        }
        ierr = nc_create(outname, NC_NOCLOBBER | cmode, &ncid);
    } else if ((CMOR_NETCDF_MODE == CMOR_APPEND_4)
               || (CMOR_NETCDF_MODE == CMOR_APPEND_3)) {
/* -------------------------------------------------------------------- */
/*      ok first let's check if the file does exists or not             */
/* -------------------------------------------------------------------- */
        fperr = NULL;
        fperr = fopen(file_suffix, "r");
        if (fperr == NULL) {

/* -------------------------------------------------------------------- */
/*      ok it does not exists... we will open as new                    */
/* -------------------------------------------------------------------- */
            ierr = nc_create(outname, NC_CLOBBER | cmode, &ncid);

        } else {                /*ok it was there already */
            bAppendMode = 1;
            ierr = fclose(fperr);
            fperr = NULL;
            copyfile(outname, file_suffix);
            ierr = nc_open(outname, NC_WRITE, &ncid);

            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) opening file: %s",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), outname);
            }

            ierr = nc_inq_dimid(ncid, "time", &i);

            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) looking for time\n! "
                    "dimension in file: %s",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), outname);
            }

            ierr = nc_inq_dimlen(ncid, i, &nctmp);
            cmor_vars[var_id].ntimes_written = (int)nctmp;

            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) looking for time\n! "
                    "dimension length in file: %s",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), outname);
            }

            ierr = nc_inq_varid(ncid, cmor_vars[var_id].id,
                                &cmor_vars[var_id].nc_var_id);

            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) looking for variable\n! "
                    "'%s' in file: %s",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr),
                    cmor_vars[var_id].id, outname);
            }

            ierr = nc_inq_varid(ncid, "time", &cmor_vars[var_id].time_nc_id);

            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) looking for time of\n! "
                    "variable '%s' in file: %s",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id, outname);
            }

/* -------------------------------------------------------------------- */
/*      ok now we need to read the first time in here                   */
/* -------------------------------------------------------------------- */
            starts[0] = 0;
            ierr = nc_get_var1_double(ncid, cmor_vars[var_id].time_nc_id,
                                      &starts[0],
                                      &cmor_vars[var_id].first_time);

            starts[0] = cmor_vars[var_id].ntimes_written - 1;
            ierr = nc_get_var1_double(ncid, cmor_vars[var_id].time_nc_id,
                                      &starts[0], &cmor_vars[var_id].last_time);

            if (cmor_tables
                [cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_table_id].
                axes[cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_axis_id].
                must_have_bounds == 1) {
                if (cmor_tables
                    [cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_table_id].
                    axes[cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_axis_id].
                    climatology == 1) {

                    snprintf(msg, CMOR_MAX_STRING, "climatology");
                    strncpy(ctmp, "climatology_bnds", CMOR_MAX_STRING);
                } else {
                    strncpy(ctmp, "time_bnds", CMOR_MAX_STRING);
                }

                ierr = nc_inq_varid(ncid, ctmp, &i);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_variadic(
                        "NetCDF Error (%i: %s) looking for time bounds\n! "
                        "of variable '%s' in file: %s",
                        CMOR_WARNING,
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id, outname);
                    ierr = NC_NOERR;
                } else {
                    cmor_vars[var_id].time_bnds_nc_id = i;
/* -------------------------------------------------------------------- */
/*      Here I need to store first/last bounds for appending issues     */
/* -------------------------------------------------------------------- */

                    starts[0] = cmor_vars[var_id].ntimes_written - 1;
                    starts[1] = 1;
                    ierr = nc_get_var1_double(ncid,
                                          cmor_vars[var_id].time_bnds_nc_id,
                                          &starts[0],
                                          &cmor_vars[var_id].last_bound);
                    starts[1] = 0;
                    ierr = nc_get_var1_double(ncid,
                                          cmor_vars[var_id].time_bnds_nc_id,
                                          &starts[0],
                                          &cmor_vars[var_id].first_bound);
                }
            }
            cmor_vars[var_id].initialized = ncid;
        }
    } else {
        cmor_handle_error_var_variadic(
            "Unknown CMOR_NETCDF_MODE file mode: %i",
            CMOR_CRITICAL, var_id,
            CMOR_NETCDF_MODE);
    }
    if (ierr != NC_NOERR) {
        cmor_handle_error_var_variadic(
            "NetCDF Error (%i: %s) creating file: %s",
            CMOR_CRITICAL, var_id,
            ierr, nc_strerror(ierr), outname);
    }
    cmor_pop_traceback();
    return (ncid);
}

/************************************************************************/
/*                      cmor_setDefaultGblAttr()                               */
/************************************************************************/
int cmor_setDefaultGblAttr(int ref_table_id)
{
    cmor_CV_def_t *CV_value;
    cmor_CV_def_t *CV_source_id;
    cmor_CV_def_t *CV_source_ids;
    cmor_CV_def_t *required_attrs;
    char source_id[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    int i, j, k;
    int isRequired;
    int ierr = 0;

    cmor_add_traceback("cmor_setDefaultGblAttr");

/* -------------------------------------------------------------------- */
/*  If this function was called without the dataset being initialized   */
/*  by cmor_dataset_json, then exit.                                    */
/* -------------------------------------------------------------------- */
    if (cmor_current_dataset.initiated ==  0) {
        cmor_pop_traceback();
        return (0);
    }

    ierr = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, source_id);
    if (ierr != 0) {
        cmor_handle_error_variadic(
            "Can't read dataset attribute %s",
            CMOR_CRITICAL, GLOBAL_ATT_SOURCE_ID);
        return (1);
    }

/* -------------------------------------------------------------------- */
/*  Find source_id entry in CV table.                                   */
/* -------------------------------------------------------------------- */
    CV_source_ids = cmor_CV_rootsearch(cmor_tables[ref_table_id].CV, CV_KEY_SOURCE_IDS);
    for(i = 0; i < CV_source_ids->nbObjects; i++){
        CV_source_id = &CV_source_ids->oValue[i];
        if (strncmp(CV_source_id->key, source_id, CMOR_MAX_STRING) == 0) {
            break;
        }
    }
    
/* -------------------------------------------------------------------- */
/*  Set default values for registered CV values.                        */
/* -------------------------------------------------------------------- */
    required_attrs = cmor_CV_rootsearch(cmor_tables[ref_table_id].CV, CV_KEY_REQUIRED_GBL_ATTRS);
    for(j = 0; j < CV_source_id->nbObjects; j++){
        CV_value = &CV_source_id->oValue[j];

        // Check if the attribute is required
        isRequired = 0;
        for (k = 0; k < required_attrs->anElements; k++) {
            if(strcmp(CV_value->key, required_attrs->aszValue[k]) == 0)
                isRequired = 1;
        }

        if(cmor_has_cur_dataset_attribute(CV_value->key) != 0){
            if(CV_value->szValue[0] != '\0'){
                ierr |= cmor_set_cur_dataset_attribute_internal(CV_value->key, CV_value->szValue, 0);
                if(strncmp(CV_value->key, GLOBAL_ATT_FURTHERINFOURL, CMOR_MAX_STRING) == 0
                     && cmor_current_dataset.furtherinfourl[0] == '\0'){
                    ierr |= cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FURTHERINFOURLTMPL, CV_value->szValue, 0);
                }
            } else if(CV_value->anElements == 1 && isRequired == 1){
                ierr |= cmor_set_cur_dataset_attribute_internal(CV_value->key, CV_value->aszValue[0], 0);
            }
        }
    }

/* -------------------------------------------------------------------- */
/*  Set further_info_url template if required and not already set.      */
/* -------------------------------------------------------------------- */
    for (k = 0; k < required_attrs->anElements; k++) {
        if(strcmp(required_attrs->aszValue[k], GLOBAL_ATT_FURTHERINFOURL) == 0
            && cmor_current_dataset.furtherinfourl[0] == '\0')
        {
            ierr |= cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FURTHERINFOURLTMPL, CMOR_DEFAULT_FURTHERURL_TEMPLATE, 0);
        }
    }

    cmor_pop_traceback();
    return ierr;

}

/************************************************************************/
/*                      cmor_setGblAttr()                               */
/************************************************************************/
int cmor_setGblAttr(int var_id)
{
    struct tm *ptr;
    time_t lt;
    char msg[CMOR_MAX_STRING];
    char timestamp[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    char words[CMOR_MAX_STRING];
    char trimword[CMOR_MAX_STRING];
    char *szToken;
    char szHistory[CMOR_MAX_STRING];
    char szTemplate[CMOR_MAX_STRING];
    int i;
    int n_matches = 10;
    regmatch_t m[n_matches];
    regex_t regex;
    int numchar;
    int nVarRefTblID;
    int ref_var_id;
    int rc;
    int ierr = 0;

    cmor_add_traceback("cmor_setGblAttr");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;
    ref_var_id = cmor_vars[var_id].ref_var_id;

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_FORCING) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_FORCING, ctmp2);
        ierr += cmor_check_forcing_validity(nVarRefTblID, ctmp2);
    }

/* -------------------------------------------------------------------- */
/*  Defined "product" from Table if not defined by users                */
/* -------------------------------------------------------------------- */

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PRODUCT) != 0) {
        strncpy(ctmp2, cmor_tables[nVarRefTblID].product, CMOR_MAX_STRING);
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_PRODUCT, ctmp2, 1);
    }

/* -------------------------------------------------------------------- */
/*      first figures out Creation time                                 */
/* -------------------------------------------------------------------- */
    lt = time(NULL);
    ptr = gmtime(&lt);
    snprintf(timestamp, CMOR_MAX_STRING, "%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ",
             ptr->tm_year + 1900, ptr->tm_mon + 1, ptr->tm_mday, ptr->tm_hour,
             ptr->tm_min, ptr->tm_sec);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_CREATION_DATE, timestamp, 0);

/* -------------------------------------------------------------------- */
/*    Set attribute Conventions for netCDF file metadata                */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%s", cmor_tables[nVarRefTblID].Conventions);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_CONVENTIONS, msg, 0);

/* -------------------------------------------------------------------- */
/*    Set attribute data_specs_versions for netCDF file (CMIP6)         */
/* -------------------------------------------------------------------- */
    if (cmor_tables[nVarRefTblID].data_specs_version[0] != '\0') {
        snprintf(msg, CMOR_MAX_STRING, "%s",
                 cmor_tables[nVarRefTblID].data_specs_version);
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_DATASPECSVERSION,
                                                msg, 0);
    }
/* -------------------------------------------------------------------- */
/*    Set attribute frequency for netCDF file (CMIP6)                   */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%s", cmor_vars[var_id].frequency);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FREQUENCY, msg, 0);
/* -------------------------------------------------------------------- */
/*    Set attribute variable_id for netCDF file (CMIP6)                 */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%s", cmor_vars[var_id].id);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VARIABLE_ID, msg, 0);

/* -------------------------------------------------------------------- */
/*    Set attribute Table_ID for netCDF file (CMIP6)                    */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%s", cmor_tables[nVarRefTblID].szTable_id);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TABLE_ID, msg, 0);

/* -------------------------------------------------------------------- */
/*    Set attribute Table_Info for netCDF file (CMIP6)                  */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "Creation Date:(%s) MD5:",
             cmor_tables[nVarRefTblID].date);

    for (i = 0; i < 16; i++) {
        sprintf(&ctmp[2 * i], "%02x", cmor_tables[nVarRefTblID].md5[i]);
    }
    ctmp[32] = '\0';
    strcat(msg, ctmp);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TABLE_INFO, msg, 0);

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, ctmp);
    } else {
        ctmp[0] = '\0';
    }
/* -------------------------------------------------------------------- */
/*    Set attribute Title for netCDF file (CMIP6)                       */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, GLOBAL_ATT_TITLE_MSG, ctmp,
             cmor_tables[nVarRefTblID].mip_era);
/* -------------------------------------------------------------------- */
/*    Change Title if not provided by user.                             */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_TITLE) != 0) {
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TITLE, msg, 0);
    }
/* -------------------------------------------------------------------- */
/*     check source and model_id are identical                          */
/* -------------------------------------------------------------------- */
    if (cmor_tables[nVarRefTblID].mip_era[0] != '\0') {
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_MIP_ERA,
                                                cmor_tables
                                                [nVarRefTblID].mip_era, 0);
    }

/* -------------------------------------------------------------------- */
/*      first check if the variable itself has a realm                  */
/* -------------------------------------------------------------------- */
    if (cmor_tables[nVarRefTblID].vars[ref_var_id].realm[0] != '\0') {
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_REALM,
                                                cmor_tables
                                                [nVarRefTblID].vars
                                                [ref_var_id].realm, 0);
    } else {
/* -------------------------------------------------------------------- */
/*      ok it didn't so we're using the value from the table            */
/* -------------------------------------------------------------------- */
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_REALM,
                                                cmor_tables[nVarRefTblID].realm,
                                                0);
    }
    cmor_generate_uuid();
    ctmp[0]='\0';
/* -------------------------------------------------------------------- */
/*     Initialize externa_variables attribute                           */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_EXTERNAL_VAR,
                                            "", 0);

/* -------------------------------------------------------------------- */
/*     Create external_variables                                        */
/* -------------------------------------------------------------------- */
    if (cmor_has_variable_attribute(var_id, VARIABLE_ATT_CELLMEASURES) == 0) {
        cmor_get_variable_attribute(var_id, VARIABLE_ATT_CELLMEASURES, ctmp);

/* -------------------------------------------------------------------- */
/*     If CELLMEASURE is set to @OPT we don't do anything               */
/* -------------------------------------------------------------------- */
        if ((strcmp(ctmp, "@OPT") == 0) || (strcmp(ctmp, "--OPT") == 0)
            || (strcmp(ctmp, "--MODEL") == 0)) {
            cmor_set_variable_attribute(var_id,
                                        VARIABLE_ATT_CELLMEASURES, 'c', "");
        } else {
/* -------------------------------------------------------------------- */
/*     Extract 2 words after "area:" or "volume:" if exist.             */
/* -------------------------------------------------------------------- */
            regcomp(&regex, EXTERNAL_VARIABLE_REGEX, REG_EXTENDED);

            rc = regexec(&regex, ctmp, n_matches, m, 0);
            if (rc == REG_NOMATCH) {
                cmor_handle_error_var_variadic(
                    "Your table (%s) does not contains CELL_MEASURES\n! "
                    "that matches 'area: <text> volume: <text>\n! "
                    "CMOR cannot build the 'external_variable' attribute.\n! "
                    "Check the following variable: '%s'.\n!",
                    CMOR_CRITICAL, var_id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    cmor_vars[var_id].id);
                regfree(&regex);
                return (-1);

            }
            words[0] = '\0';
            ctmp2[0] = '\0';
            for (i = 0; i < n_matches; i++) {
                numchar = (int)m[i].rm_eo - (int)m[i].rm_so;
/* -------------------------------------------------------------------- */
/*     If rm_so is negative, there is not more matches.                 */
/* -------------------------------------------------------------------- */
                if (((int)m[i].rm_so < 0) || (numchar == 0)) {
                    break;
                }

                strncpy(words, ctmp + m[i].rm_so, numchar);
                words[numchar] = '\0';
                if (strstr(words, ":") != 0) {
                    continue;
                }
                cmor_trim_string(words, trimword);
                if ((strcmp(trimword, AREA) != 0)
                    && (strcmp(trimword, VOLUME) != 0)
                    && (strlen(trimword) != strlen(ctmp))) {
/* -------------------------------------------------------------------- */
/*      Rejects all word area and volume.                               */
/* -------------------------------------------------------------------- */
                    if (ctmp2[0] == '\0') {
                        strncat(ctmp2, trimword, numchar);
                    } else {
                        strcat(ctmp2, " ");
                        strncat(ctmp2, trimword, numchar);
                    }
                }
            }
            cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_EXTERNAL_VAR,
                                                    ctmp2, 0);
            regfree(&regex);
        }
    }                           // Remove regular expression to compare strings.

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION_ID) == 0) {
        ierr += cmor_CV_setInstitution(cmor_tables[nVarRefTblID].CV);
    }

    ierr += cmor_CV_checkFurtherInfoURL(nVarRefTblID);

    if (cmor_has_cur_dataset_attribute(GLOBAL_IS_CMIP6) == 0) {
        ierr += cmor_CV_checkSourceID(cmor_tables[nVarRefTblID].CV);
        ierr += cmor_CV_checkExperiment(cmor_tables[nVarRefTblID].CV);
        ierr += cmor_CV_checkGrids(cmor_tables[nVarRefTblID].CV);
        ierr += cmor_CV_checkParentExpID(cmor_tables[nVarRefTblID].CV);
        ierr += cmor_CV_checkSubExpID(cmor_tables[nVarRefTblID].CV);
    }
    //
    // Set user defined attributes and explicit {} sets.
    //
    ierr += cmor_CV_checkGblAttributes(cmor_tables[nVarRefTblID].CV);
    //
    // Copy block to ensure all attributes are set for obs4MIPs
    // especially (source_label)
    //
    if ( cmor_current_dataset.furtherinfourl[0] != '\0') {
        ierr += cmor_CV_checkSourceID(cmor_tables[nVarRefTblID].CV);
    }

    ierr += cmor_CV_checkISOTime(GLOBAL_ATT_CREATION_DATE);
    if (did_history == 0) {
        szHistory[0] ='\0';
/* -------------------------------------------------------------------- */
/*    Create History metadata from template                             */
/* -------------------------------------------------------------------- */
        strcpy(szTemplate, cmor_current_dataset.history_template);
        ierr += cmor_CreateFromTemplate(nVarRefTblID, szTemplate, szHistory, "");
        snprintf(ctmp, CMOR_MAX_STRING,
                 szHistory,
                 timestamp);

        if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_HISTORY) == 0) {
            cmor_get_cur_dataset_attribute(GLOBAL_ATT_HISTORY, msg);
            snprintf(ctmp2, CMOR_MAX_STRING, "%s;\n%s", ctmp, msg);
            strncpy(ctmp, ctmp2, CMOR_MAX_STRING);
        }
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_HISTORY, ctmp, 0);
        did_history = 1;
    }

    return (ierr);
}

/************************************************************************/
/*                      cmor_writeGblAttr()                             */
/************************************************************************/
int cmor_writeGblAttr(int var_id, int ncid, int ncafid)
{
    char msg[CMOR_MAX_STRING];
    int ierr;
    float afloat, d;

    int nVarRefTblID;

    cmor_add_traceback("cmor_writeGblAttr");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;
    cmor_write_all_attributes(ncid, ncafid, var_id);

/* -------------------------------------------------------------------- */
/*      check table cf version vs ours                                  */
/* -------------------------------------------------------------------- */
    afloat = CMOR_CF_VERSION_MAJOR;
    d = CMOR_CF_VERSION_MINOR;
    while (d > 1.) {
        d /= 10.;
    }
    afloat += d;

    if (cmor_tables[nVarRefTblID].cf_version > afloat) {
        cmor_handle_error_variadic(
            "Your table (%s) claims to enforce CF version %f but\n! "
            "this version of the library is designed for CF up\n! "
            "to: %i.%i, you were writing variable: %s\n! ",
            CMOR_WARNING,
            cmor_tables[nVarRefTblID].szTable_id,
            cmor_tables[nVarRefTblID].cf_version, CMOR_CF_VERSION_MAJOR,
            CMOR_CF_VERSION_MINOR, cmor_vars[var_id].id);
    }
/* -------------------------------------------------------------------- */
/*      cmor_ver                                                        */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%i.%i.%i", CMOR_VERSION_MAJOR,
             CMOR_VERSION_MINOR, CMOR_VERSION_PATCH);
    ierr = nc_put_att_text(ncid, NC_GLOBAL, GLOBAL_ATT_CMORVERSION,
                           strlen(msg) + 1, msg);
    if (ierr != NC_NOERR) {
        cmor_handle_error_var_variadic(
            "NetCDF error (%i: %s) writing variable %s (table: %s)\n! "
            "global att cmor_version (%f)",
            CMOR_CRITICAL, var_id,
            ierr, nc_strerror(ierr),
            cmor_vars[var_id].id, cmor_tables[nVarRefTblID].szTable_id,
            afloat);
    }

    if (ncid != ncafid) {

/* -------------------------------------------------------------------- */
/*      cmor_ver                                                        */
/* -------------------------------------------------------------------- */
        ierr = nc_put_att_text(ncid, NC_GLOBAL, GLOBAL_ATT_CMORVERSION,
                               strlen(msg) + 1, msg);
        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "NetCDF error (%i: %s) writing variable %s\n! "
                "(table: %s) global att cmor_version (%f)",
                CMOR_CRITICAL, var_id,
                ierr,
                nc_strerror(ierr), cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id, afloat);
        }
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                        cmor_attNameCmp()                             */
/************************************************************************/
int cmor_attNameCmp(const void *v1, const void *v2)
{
    const attributes_def *c1 = v1;
    const attributes_def *c2 = v2;
    return (strcmp(c1->names, c2->names));
}

/************************************************************************/
/*                     cmor_generate_uuid()                             */
/************************************************************************/
void cmor_generate_uuid()
{
    uuid_t myuuid;
    char myuuid_str[37]; // 36 characters + '\0'
    char value[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_generate_uuid");

/* -------------------------------------------------------------------- */
/*      generates a new unique id                                       */
/* -------------------------------------------------------------------- */
    uuid_generate(myuuid);

/* -------------------------------------------------------------------- */
/*      Write tracking_id and tracking_prefix                           */
/* -------------------------------------------------------------------- */
    uuid_unparse(myuuid, myuuid_str);

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_TRACKING_PREFIX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_TRACKING_PREFIX, value);

        strncpy(cmor_current_dataset.tracking_id, value, CMOR_MAX_STRING);
        strcat(cmor_current_dataset.tracking_id, "/");
        strcat(cmor_current_dataset.tracking_id, myuuid_str);
    } else {
        strncpy(cmor_current_dataset.tracking_id, myuuid_str,
                CMOR_MAX_STRING);
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TRACKING_ID,
                                            cmor_current_dataset.tracking_id,
                                            0);
    cmor_pop_traceback();

}

/************************************************************************/
/*                  cmor_write_all_atributes()                          */
/************************************************************************/
void cmor_write_all_attributes(int ncid, int ncafid, int var_id)
{
    int ierr;
    char msg[CMOR_MAX_STRING];
    char value[CMOR_MAX_STRING];
    double tmps[2];
    int i;
    int nVarRefTblID;
    int itmp2;
    int rc;

    cmor_add_traceback("cmor_write_all_attributes");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

    qsort(cmor_current_dataset.attributes, cmor_current_dataset.nattributes,
          sizeof(struct attributes), cmor_attNameCmp);

    for (i = 0; i < cmor_current_dataset.nattributes; i++) {
/* -------------------------------------------------------------------- */
/* Skip "calendar" global attribute                                     */
/* -------------------------------------------------------------------- */
        if (strcmp(cmor_current_dataset.attributes[i].names,
                   GLOBAL_ATT_CALENDAR) == 0) {
            continue;
        }
/* -------------------------------------------------------------------- */
/* Skip "tracking_prefix" global attribute                              */
/* -------------------------------------------------------------------- */
        if (strcmp(cmor_current_dataset.attributes[i].names,
                   GLOBAL_ATT_TRACKING_PREFIX) == 0) {
            continue;
        }
/* -------------------------------------------------------------------- */
/* Write license last, not now!!                                        */
/* -------------------------------------------------------------------- */
        if (strcmp(cmor_current_dataset.attributes[i].names,
                   GLOBAL_ATT_LICENSE) == 0) {
            continue;
        }
/* -------------------------------------------------------------------- */
/*  Write Branch_Time as double attribute                               */
/* -------------------------------------------------------------------- */

        rc = strncmp(cmor_current_dataset.attributes[i].names,
                     GLOBAL_ATT_BRANCH_TIME, 11);
/* -------------------------------------------------------------------- */
/*  matches "branch_time" and "branch_time_something"                   */
/* -------------------------------------------------------------------- */
        if (rc == 0) {
            sscanf(cmor_current_dataset.attributes[i].values, "%lf", &tmps[0]);
            ierr = nc_put_att_double(ncid, NC_GLOBAL,
                                     cmor_current_dataset.attributes[i].names,
                                     NC_DOUBLE, 1, &tmps[0]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF error (%i: %s) for variable %s\n! "
                    "(table: %s)  writing global att: %s (%s)\n! ",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    cmor_current_dataset.attributes[i].names,
                    cmor_current_dataset.attributes[i].values);
            }
            if (ncid != ncafid) {
                ierr = nc_put_att_double(ncafid, NC_GLOBAL,
                                         cmor_current_dataset.attributes[i].
                                         names, NC_DOUBLE, 1, &tmps[0]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF error (%i: %s) for variable\n! "
                        "%s (table: %s), writing global att\n! "
                        "to metafile: %s (%s)",
                        CMOR_CRITICAL, var_id,
                        ierr,
                        nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id,
                        cmor_current_dataset.attributes[i].names,
                        cmor_current_dataset.attributes[i].values);
                }
            }
        } else if ((strcmp(cmor_current_dataset.attributes[i].names,
                           GLOBAL_ATT_REALIZATION) == 0) ||
                   (strcmp(cmor_current_dataset.attributes[i].names,
                           GLOBAL_ATT_INITIA_IDX) == 0) ||
                   (strcmp(cmor_current_dataset.attributes[i].names,
                           GLOBAL_ATT_PHYSICS_IDX) == 0) ||
                   (strcmp(cmor_current_dataset.attributes[i].names,
                           GLOBAL_ATT_FORCING_IDX) == 0)) {
            sscanf(cmor_current_dataset.attributes[i].values, "%d", &itmp2);
            ierr = nc_put_att_int(ncid, NC_GLOBAL,
                                  cmor_current_dataset.attributes[i].names,
                                  NC_INT, 1, &itmp2);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF error (%i: %s) for variable %s\n! "
                    "(table: %s)  writing global att: %s (%s)\n! ",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    cmor_current_dataset.attributes[i].names,
                    cmor_current_dataset.attributes[i].values);
            }
        } else {
            itmp2 = strlen(cmor_current_dataset.attributes[i].values);
            if (itmp2 < CMOR_DEF_ATT_STR_LEN) {
                int nNbAttrs =
                  strlen(cmor_current_dataset.attributes[i].values);
                for (itmp2 = nNbAttrs; itmp2 < CMOR_DEF_ATT_STR_LEN; itmp2++) {
                    cmor_current_dataset.attributes[i].values[itmp2] = '\0';
                }
                itmp2 = CMOR_DEF_ATT_STR_LEN;
            }
/* -------------------------------------------------------------------- */
/*  Write all "text" attributes                                         */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      Skip attributes starting with "_"                               */
/* -------------------------------------------------------------------- */
            if ((cmor_current_dataset.attributes[i].names[0] != '_')
                    && (cmor_current_dataset.attributes[i].values[0] != '\0')) {
                ierr = nc_put_att_text(ncid, NC_GLOBAL,
                                       cmor_current_dataset.attributes[i].names,
                                       itmp2,
                                       cmor_current_dataset.attributes[i].
                                       values);

                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF error (%i: %s) for variable %s\n! "
                        "(table: %s)  writing global att: %s (%s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id,
                        cmor_current_dataset.attributes[i].names,
                        cmor_current_dataset.attributes[i].values);
                }
                if (ncid != ncafid) {
                    ierr = nc_put_att_text(ncafid, NC_GLOBAL,
                                           cmor_current_dataset.attributes[i].
                                           names, itmp2,
                                           cmor_current_dataset.attributes[i].
                                           values);
                    if (ierr != NC_NOERR) {
                        cmor_handle_error_var_variadic(
                            "NetCDF error (%i: %s) for variable %s\n! "
                            "(table %s), writing global att to\n! "
                            "metafile: %s (%s)",
                            CMOR_CRITICAL, var_id,
                            ierr,
                            nc_strerror(ierr), cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id,
                            cmor_current_dataset.attributes[i].names,
                            cmor_current_dataset.attributes[i].values);
                    }
                }
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      Write license attribute                                         */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_LICENSE) == 0) {

        cmor_get_cur_dataset_attribute(GLOBAL_ATT_LICENSE, value);
        itmp2 = strlen(value);

        ierr = nc_put_att_text(ncid, NC_GLOBAL, GLOBAL_ATT_LICENSE, itmp2,
                               value);

        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "NetCDF error (%i: %s) for variable %s\n! "
                "(table: %s)  writing global att: %s (%s)",
                CMOR_CRITICAL, var_id,
                ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id,
                GLOBAL_ATT_LICENSE, value);
        }
        if (ncid != ncafid) {
            ierr = nc_put_att_text(ncafid, NC_GLOBAL,
                                   GLOBAL_ATT_LICENSE, itmp2, value);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF error (%i: %s) for variable %s\n! "
                    "(table %s), writing global att to\n! "
                    "metafile: %s (%s)",
                    CMOR_CRITICAL, var_id,
                    ierr,
                    nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    GLOBAL_ATT_LICENSE, value);
            }
        }
    }
    cmor_pop_traceback();
}

/************************************************************************/
/*                cmor_define_dimensions()                              */
/************************************************************************/
void cmor_define_dimensions(int var_id, int ncid,
                            int ncafid, double *time_bounds,
                            int *nc_dim,
                            int *nc_vars, int *nc_bnds_vars,
                            int *nc_vars_af,
                            size_t * nc_dim_chunking, int *dim_bnds,
                            int *zfactors, int *nc_zfactors,
                            int *nc_dim_af, int *nzfactors)
{
    int i, j, k, l, n;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    char ctmp3[CMOR_MAX_STRING];

    int ierr;
    int tmp_dims[2];
    int dims_bnds_ids[2];
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int ics, icd, icdl, icz;
    int itmpmsg, itmp2, itmp3;
    int maxStrLen;

    cmor_add_traceback("cmor_define_dimensions");

    maxStrLen = 0;
    for (i = 0; i < cmor_vars[var_id].ndims; i++) {
/* -------------------------------------------------------------------- */
/*      did we flip that guy?                                           */
/* -------------------------------------------------------------------- */
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].revert == -1) {
            sprintf(msg, "Inverted axis: %s",
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id);
/* -------------------------------------------------------------------- */
/*      fiddle to avoid duplicated effort here if it's already inverted */
/* -------------------------------------------------------------------- */
            if (!cmor_history_contains(var_id, msg)) {
                cmor_update_history(var_id, msg);
            }
        }
        int nAxisID = cmor_vars[var_id].axes_ids[i];
/* -------------------------------------------------------------------- */
/*      Axis length                                                     */
/* -------------------------------------------------------------------- */
        j = cmor_axes[nAxisID].length;
        if ((i == 0) && (cmor_axes[nAxisID].axis == 'T'))
            j = NC_UNLIMITED;

        if ((cmor_axes[nAxisID].axis == 'X')
            || (cmor_axes[nAxisID].axis == 'Y')) {
            nc_dim_chunking[i] = j;
        } else if (cmor_axes[nAxisID].isgridaxis == 1) {
            nc_dim_chunking[i] = j;
        } else {
            nc_dim_chunking[i] = 1;
        }

        ierr = nc_def_dim(ncid, cmor_axes[nAxisID].id, j, &nc_dim[i]);
        if (ierr != NC_NOERR) {
            ierr = nc_enddef(ncid);
            cmor_handle_error_var_variadic(
                "NetCDF error (%i:%s) for dimension definition of\n! "
                "axis: %s (%i), for variable %i (%s, table: %s)",
                CMOR_CRITICAL, var_id,
                ierr, nc_strerror(ierr), cmor_axes[nAxisID].id, nAxisID,
                var_id, cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id);
        }
        nc_dim_af[i] = nc_dim[i];
        if (ncid != ncafid) {
            ierr = nc_def_dim(ncafid, cmor_axes[nAxisID].id, j, &nc_dim_af[i]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF error (%i: %s) for dimension definition\n! "
                    "of axis: %s (%i) in metafile, variable %s "
                    "(table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr),
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id, i,
                    cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            }
        }
/* -------------------------------------------------------------------- */
/*      find maximum string length for string dimensions                */
/* -------------------------------------------------------------------- */
        if (cmor_axes[nAxisID].cvalues != NULL) {
            for (j = 0; j < cmor_axes[nAxisID].length; j++) {
                strncpy(msg, cmor_axes[nAxisID].cvalues[j], CMOR_MAX_STRING);
                k = strlen(msg);
                if (k > maxStrLen) {
                    maxStrLen = k;
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      find maximum string length for singleton dimension variables    */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        j = cmor_vars[var_id].singleton_ids[i];
        if (j != -1) {
            if (cmor_tables[cmor_axes[j].ref_table_id].axes
                [cmor_axes[j].ref_axis_id].type == 'c') {
                k = strlen(cmor_tables[cmor_axes[j].ref_table_id].axes
                            [cmor_axes[j].ref_axis_id].cvalue);
                if (k > maxStrLen) {
                    maxStrLen = k;
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      creates the bounds dim (only in metafile?)                      */
/* -------------------------------------------------------------------- */
    ierr = nc_def_dim(ncafid, "bnds", 2, dim_bnds);
    if (ierr != NC_NOERR) {
        cmor_handle_error_var_variadic(
                 "NC error (%i: %s), error creating bnds dimension to\n! "
                 "metafile, variable %s (table: %s)",
                 CMOR_CRITICAL, var_id,
                 ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                 cmor_tables[nVarRefTblID].szTable_id);
    }

/* -------------------------------------------------------------------- */
/*      Now define the variable corresponding to                        */
/*      store the dimensions values                                     */
/* -------------------------------------------------------------------- */
    for (i = 0; i < cmor_vars[var_id].ndims; i++) {
        cmor_axis_t *pAxis;
        pAxis = &cmor_axes[cmor_vars[var_id].axes_ids[i]];
        if (pAxis->store_in_netcdf == 0)
            continue;

        if (pAxis->cvalues == NULL) {
/* -------------------------------------------------------------------- */
/*       first we need to figure out the output type                    */
/* -------------------------------------------------------------------- */
            switch (cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].
                    type) {

              case ('f'):
                  j = NC_FLOAT;
                  break;
              case ('d'):
                  j = NC_DOUBLE;
                  break;
              case ('i'):
                  j = NC_INT;
                  break;
              default:
                  j = NC_DOUBLE;
                  break;
            }
            ierr = nc_def_var(ncid, pAxis->id, j, 1, &nc_dim[i], &nc_vars[i]);

            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) for variable %s\n! "
                    "(table: %s) error defining dim var: %i (%s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
            }
            //
            // Define Chunking if NETCDF4
            //
            cmor_set_chunking(var_id, nVarRefTblID, nc_dim_chunking);
            if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
                if (strcmp(pAxis->id, "time") == 0) {
                    ierr = nc_def_var_chunking(ncid, nc_vars[i], NC_CHUNKED,
                                               NULL);
                } else {
                    ierr = nc_def_var_chunking(ncid, nc_vars[i], NC_CONTIGUOUS,
                                               &nc_dim_chunking[0]);
                }
            }
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) for variable %s\n! "
                    "(table: %s) error defining dim var: %i (%s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
            }
            nc_vars_af[i] = nc_vars[i];
            if (ncid != ncafid) {
                ierr = nc_def_var(ncafid, pAxis->id, j, 1, &nc_dim_af[i],
                                  &nc_vars_af[i]);

                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s ) for variable %s\n! "
                        "(table: %s) error defining dim var: %i\n! "
                        "(%s) in metafile",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id, i,
                        pAxis->id);
                }

            }

        } else {
/* -------------------------------------------------------------------- */
/*      ok at this point i'm assuming only 1 string dimension!          */
/*      might need to be revised                                        */
/*      so i only create 1 strlen dim                                   */
/*      first need to figure out if the "region name is defined         */
/* -------------------------------------------------------------------- */
            strcpy(ctmp,
                   cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].
                   cname);

            if (ctmp[0] == '\0') {
                strcpy(ctmp, "sector");
            }

            if (cmor_has_variable_attribute(var_id, "coordinates") == 0) {
                cmor_get_variable_attribute(var_id, "coordinates", msg);
                l = 0;
                if(strlen(msg) >= strlen(ctmp)) {
                    for (j = 0; j < strlen(msg) - strlen(ctmp) + 1; j++) {
                        if (strncmp(ctmp, &msg[j], strlen(ctmp)) == 0) {
                            l = 1;
                            break;
                        }
                    }
                }

                if (l == 0) {
                    strncat(msg, " ", CMOR_MAX_STRING - strlen(msg));
                    strncat(msg, ctmp, CMOR_MAX_STRING - strlen(msg));
                }
            } else {
                strncpy(msg, ctmp, CMOR_MAX_STRING);
            }

            cmor_set_variable_attribute_internal(var_id,
                                                 VARIABLE_ATT_COORDINATES,
                                                 'c', msg);

/* -------------------------------------------------------------------- */
/*      ok so now i can create the dummy dim strlen                     */
/* -------------------------------------------------------------------- */
            if(nc_inq_dimid(ncid, "strlen", &tmp_dims[1]) != NC_NOERR) {
                ierr =
                    nc_def_dim(ncid, "strlen", maxStrLen, &tmp_dims[1]);
            }
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF error (%i: %s) for dummy 'strlen'\n! "
                    "dimension definition of axis: %s (%i) in\n! "
                    "metafile, while writing variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), pAxis->id, i,
                    cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            }
            tmp_dims[0] = nc_dim[i];
            ierr = nc_def_var(ncid, ctmp, NC_CHAR, 2, &tmp_dims[0],
                              &nc_vars[i]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) for variable %s\n! "
                    "(table: %s) error defining dim var: %i (%s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
            }

            nc_vars_af[i] = nc_vars[i];

            if (ncid != ncafid) {

                if(nc_inq_dimid(ncafid, "strlen", &tmp_dims[1]) != NC_NOERR) {
                    ierr =
                        nc_def_dim(ncafid, "strlen", maxStrLen, &tmp_dims[1]);
                }

                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF error (%i: %s) for dummy 'strlen'\n! "
                        "dimension definition of axis: %s (%i) in\n! "
                        "metafile, while writing variable %s "
                        "(table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        pAxis->id, i, cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                }
                tmp_dims[0] = nc_dim_af[i];

                ierr = nc_def_var(ncafid, ctmp, NC_CHAR, 1, &tmp_dims[0],
                                  &nc_vars_af[i]);

                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) for variable %s\n! "
                        "(table: %s) error defining dim var:\n! "
                        "%i (%s) in metafile",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id, i,
                        pAxis->id);
                }
            }
        }
/* -------------------------------------------------------------------- */
/*      ok do we have bounds on this axis?                              */
/* -------------------------------------------------------------------- */
        if ((pAxis->bounds != NULL) || ((i == 0) && (time_bounds != NULL))) {
            strncpy(ctmp, pAxis->id, CMOR_MAX_STRING);
            strncat(ctmp, "_bnds", CMOR_MAX_STRING - strlen(ctmp));
            snprintf(msg, CMOR_MAX_STRING, "bounds");
            if (i == 0) {
/* -------------------------------------------------------------------- */
/*      Ok here we need to see if it is a climatological                */
/*      variable in order to change                                     */
/*      the "bounds" attribute into "climatology"                       */
/* -------------------------------------------------------------------- */
                if (cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].
                    climatology == 1) {
                    snprintf(msg, CMOR_MAX_STRING, "climatology");
                    strncpy(ctmp, "climatology_bnds", CMOR_MAX_STRING);
                }
            }
            dims_bnds_ids[0] = nc_dim[i];
            dims_bnds_ids[1] = *dim_bnds;
            switch (cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].
                    type) {
              case ('f'):
                  j = NC_FLOAT;
                  break;
              case ('d'):
                  j = NC_DOUBLE;
                  break;
              case ('i'):
                  j = NC_INT;
                  break;
              default:
                  j = NC_DOUBLE;
                  break;
            }
            ierr = nc_def_var(ncafid, ctmp, j, 2, &dims_bnds_ids[0],
                              &nc_bnds_vars[i]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) for variable %s\n! "
                    "(table: %s) error defining bounds dim var: %i (%s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
            }

/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
            if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
                cmor_var_t *pVar;

                pVar = &cmor_vars[var_id];

                ics = pVar->shuffle;
                icd = pVar->deflate;
                icdl = pVar->deflate_level;
                icz = pVar->zstandard_level;

                if (icd != 0) {
                    ierr |= nc_def_var_deflate(ncafid, nc_bnds_vars[i],
                                            ics, icd, icdl);
                } else {
                    ierr |= nc_def_var_deflate(ncafid, nc_bnds_vars[i],
                                            ics, 0, 0);
                    ierr |= nc_def_var_zstandard(ncafid, nc_bnds_vars[i],
                                            icz);
                }
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NCError (%i: %s) defining compression\n! "
                        "parameters for bounds variable %s for\n! "
                        "variable '%s' (table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr,
                        nc_strerror(ierr), ctmp, cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                }

            }
/* -------------------------------------------------------------------- */
/* sets the bounds attribute of parent var                              */
/* -------------------------------------------------------------------- */

            if (i == 0)
                cmor_vars[var_id].time_bnds_nc_id = nc_bnds_vars[i];
            ierr = nc_put_att_text(ncafid, nc_vars[i], msg, strlen(ctmp) + 1,
                                   ctmp);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) for variable %s\n! "
                    "(table: %s) error defining bounds attribute\n! "
                    "var: %i (%s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr),
                    cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
            }
        }
/* -------------------------------------------------------------------- */
/*      Creates attribute related to that axis                          */
/* -------------------------------------------------------------------- */

        for (j = 0; j < cmor_axes[cmor_vars[var_id].axes_ids[i]].nattributes;
             j++) {
            if (strcmp(cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                       "z_factors") == 0) {
/* -------------------------------------------------------------------- */
/*      ok this part checks for z_factor things                         */
/*      creates the formula terms attriubte                             */
/* -------------------------------------------------------------------- */
                strncpy(msg,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].
                        attributes_values_char[j], CMOR_MAX_STRING);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncid, nc_vars[i], "formula_terms", n,
                                       msg);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF error (%i: %s) writing formula term "
                        "att (%s) for axis %i (%s), variable %s "
                        "(table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr), msg,
                        i, cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                }

                if (ncid != ncafid) {
                    ierr = nc_put_att_text(ncafid, nc_vars_af[i],
                                           "formula_terms", n, msg);
                    if (ierr != NC_NOERR) {
                        cmor_handle_error_var_variadic(
                            "NetCDF error (%i: %s) writing formula "
                            "term att (%s) for axis %i (%s), variable "
                            "%s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            ierr, nc_strerror(ierr), msg, i,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    }
                }
                ierr = cmor_define_zfactors_vars(var_id, ncafid, &nc_dim_af[0],
                                                 msg, nzfactors, &zfactors[0],
                                                 &nc_zfactors[0], i, -1);
                if (ierr != 0) {
                    break;
                }
            } else
              if (strcmp
                  (cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                   "z_bounds_factors") == 0) {
                cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i],
                                        "formula", 'c', &msg);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "formula", n,
                                       msg);
                cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i],
                                        "standard_name", 'c', &msg);
                n = strlen(msg);
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "standard_name",
                                       n, msg);
                cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i], "units",
                                        'c', &msg);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "units", n,
                                       msg);
                /*formula terms */
                strncpy(msg,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].
                        attributes_values_char[j], CMOR_MAX_STRING);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "formula_terms",
                                       n, msg);
                ierr = cmor_define_zfactors_vars(var_id, ncafid, nc_dim, msg,
                                                 nzfactors, &zfactors[0],
                                                 &nc_zfactors[0], i, *dim_bnds);
            } else
              if (strcmp
                  (cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                   "interval") == 0) {
                if (cmor_has_variable_attribute(var_id, "cell_methods") == 0) {
                    cmor_get_variable_attribute(var_id, "cell_methods", msg);
                } else {
                    strcpy(msg, "");
                }
                strncpy(ctmp, cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                        CMOR_MAX_STRING);
                strncat(ctmp, ":", CMOR_MAX_STRING - strlen(ctmp));
                icd = strlen(ctmp);
                itmpmsg = strlen(msg);
                for (ics = 0; ics < (itmpmsg - icd); ics++) {
                    for (icdl = 0; icdl < icd; icdl++) {
                        ctmp2[icdl] = msg[ics + icdl];
                        ctmp2[icdl + 1] = '\0';
                    }
                    if (strcmp(ctmp2, ctmp) == 0) {
                        itmp2 = strlen(ctmp);
                        for (icdl = 0; icdl < (ics + itmp2 + 1); icdl++) {
                            ctmp2[icdl] = msg[icdl];
                        }
                        while ((msg[icdl] != ' ') && (msg[icdl] != '\0')) {
                            ctmp2[icdl] = msg[icdl];
                            icdl++;
                        }
                        ctmp2[icdl] = '\0';

                        icd = strlen(ctmp2);
/* -------------------------------------------------------------------- */
/*      ok now we need to know if the user passed an                    */
/*      interval or not in order to add it                              */
/* -------------------------------------------------------------------- */
                        cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i],
                                                "interval", 'c', ctmp);

                        cmor_trim_string(ctmp, ctmp3);

                        if (strcmp(ctmp3, "") != 0) {
                            strncat(ctmp2, " (interval: ",
                                    CMOR_MAX_STRING - strlen(ctmp2));
                            strncat(ctmp2, ctmp,
                                    CMOR_MAX_STRING - strlen(ctmp2));
                            strncat(ctmp2, ")",
                                    CMOR_MAX_STRING - strlen(ctmp2));
                        }

                        ierr = strlen(ctmp2) - icd;
                        itmp3 = strlen(msg);
                        for (icdl = icd; icdl < itmp3; icdl++) {
                            ctmp2[icdl + ierr] = msg[icdl];
                            ctmp2[icdl + 1 + ierr] = '\0';
                        }
                        cmor_set_variable_attribute_internal(var_id,
                                                             "cell_methods",
                                                             'c', ctmp2);
                        break;
                    }
                }
            } else {
                if ((cmor_tables
                     [cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_table_id].
                     axes[cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_axis_id].
                     type == 'c')
                    &&
                    (strcmp
                     (cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                      "units") == 0)) {
/* -------------------------------------------------------------------- */
/*      passing we do not want the units attribute                      */
/* -------------------------------------------------------------------- */
                } else {
                    cmor_axis_t *pAxis;
                    pAxis = &cmor_axes[cmor_vars[var_id].axes_ids[i]];

                    if (pAxis->attributes_type[j] == 'c') {
                        ierr = cmor_put_nc_char_attribute(ncid, nc_vars[i],
                                                          pAxis->attributes[j],
                                                          pAxis->
                                                          attributes_values_char
                                                          [j],
                                                          cmor_vars[var_id].id);

                        if (ncid != ncafid) {
                            ierr = cmor_put_nc_char_attribute(ncafid,
                                                              nc_vars_af[i],
                                                              pAxis->attributes
                                                              [j],
                                                              pAxis->
                                                              attributes_values_char
                                                              [j],
                                                              cmor_vars[var_id].
                                                              id);
                        }
                    } else {
                        ierr = cmor_put_nc_num_attribute(ncid, nc_vars[i],
                                                         pAxis->attributes[j],
                                                         pAxis->attributes_type
                                                         [j],
                                                         pAxis->
                                                         attributes_values_num
                                                         [j],
                                                         cmor_vars[var_id].id);

                        if (ncid != ncafid) {
                            ierr = cmor_put_nc_num_attribute(ncafid,
                                                             nc_vars_af[i],
                                                             pAxis->attributes
                                                             [j],
                                                             pAxis->
                                                             attributes_type[j],
                                                             pAxis->
                                                             attributes_values_num
                                                             [j],
                                                             cmor_vars[var_id].
                                                             id);
                        }
                    }
                }
            }
        }
    }
    cmor_pop_traceback();
}

/************************************************************************/
/*                         cmor_grids_def()                             */
/************************************************************************/
int cmor_grids_def(int var_id, int nGridID, int ncafid, int *nc_dim_af,
                   int *nc_associated_vars)
{
    int ierr;
    int m;
    char msg[CMOR_MAX_STRING];
    double tmps[2];
    int i, j, k, l;
    int nc_dims_associated[CMOR_MAX_AXES];
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int m2[5];
    int *int_list = NULL;
    char mtype;
    int nelts;
    int ics, icd, icdl, icz;

    cmor_add_traceback("cmor_grids_def");
/* -------------------------------------------------------------------- */
/*      first of all checks for grid_mapping                            */
/* -------------------------------------------------------------------- */

    if (strcmp(cmor_grids[nGridID].mapping, "") != 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to create this dummy variable                        */
/*      that contains all the info                                      */
/* -------------------------------------------------------------------- */

        cmor_set_variable_attribute_internal(var_id,
                                             VARIALBE_ATT_GRIDMAPPING,
                                             'c', cmor_grids[nGridID].mapping);

        ierr = nc_def_var(ncafid, cmor_grids[nGridID].mapping, NC_INT, 0,
                          &nc_dims_associated[0], &m);

        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "NetCDF error (%i: %s) while defining\n! "
                "associated grid mapping variable %s for\n! "
                "variable %s (table: %s)",
                CMOR_CRITICAL, var_id,
                ierr, nc_strerror(ierr),
                cmor_grids[nGridID].mapping, cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id);
        }
/* -------------------------------------------------------------------- */
/*      Creates attributes related to that variable                     */
/* -------------------------------------------------------------------- */
        ierr = cmor_put_nc_char_attribute(ncafid, m, "grid_mapping_name",
                                          cmor_grids[nGridID].mapping,
                                          cmor_vars[var_id].id);
        for (k = 0; k < cmor_grids[cmor_vars[var_id].grid_id].nattributes; k++) {
            if (strcmp(cmor_grids[nGridID].attributes_names[k],
                       "standard_parallel1") == 0
                || strcmp(cmor_grids[nGridID].attributes_names[k],
                          "standard_parallel2") == 0) {

                i = -nGridID - CMOR_MAX_GRIDS;
                if ((cmor_has_grid_attribute(i, "standard_parallel1") == 0)
                    && (cmor_has_grid_attribute(i, "standard_parallel2")
                        == 0)) {
                    cmor_get_grid_attribute(i, "standard_parallel1", &tmps[0]);
                    cmor_get_grid_attribute(i, "standard_parallel2", &tmps[1]);
                    ierr = nc_put_att_double(ncafid, m, "standard_parallel",
                                             NC_DOUBLE, 2, &tmps[0]);
                } else if (cmor_has_grid_attribute(i, "standard_parallel1")
                           == 0) {
                    cmor_get_grid_attribute(i, "standard_parallel1", &tmps[0]);
                    ierr = nc_put_att_double(ncafid, m, "standard_parallel",
                                             NC_DOUBLE, 1, &tmps[0]);
                } else {
                    cmor_get_grid_attribute(i, "standard_parallel2", &tmps[0]);
                    ierr = nc_put_att_double(ncafid, m, "standard_parallel",
                                             NC_DOUBLE, 1, &tmps[0]);
                }
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) writing\n! "
                        "standard_parallel to file, variable:\n! "
                        "%s (table: %s)",
                        CMOR_NORMAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                    cmor_pop_traceback();
                    return (1);
                }
            } else {
                ierr = cmor_put_nc_num_attribute(ncafid, m,
                                                 cmor_grids
                                                 [nGridID].attributes_names[k],
                                                 'd',
                                                 cmor_grids
                                                 [nGridID].attributes_values[k],
                                                 cmor_grids[nGridID].mapping);
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      Preps the marker for vertices dimensions                        */
/* -------------------------------------------------------------------- */

    m = 0;
/* -------------------------------------------------------------------- */
/*      At this point creates the associated variables                  */
/*      all is done is associated file                                  */
/* -------------------------------------------------------------------- */
    for (i = 0; i < 5; i++) {
        m2[i] = 0;
        j = cmor_grids[nGridID].associated_variables[i];

        if (j != -1) {
/* -------------------------------------------------------------------- */
/*      ok we need to define this variable                              */
/* -------------------------------------------------------------------- */

            l = 0;
/* -------------------------------------------------------------------- */
/*      first we need to figure out the actual                          */
/*      grid dimensions and their netcdf eq                             */
/* -------------------------------------------------------------------- */

            for (k = 0; k < cmor_vars[var_id].ndims; k++) {
                if (cmor_axes[cmor_vars[var_id].axes_ids[k]].isgridaxis == 1) {
                    nc_dims_associated[l] = nc_dim_af[k];

                    if (m2[i] == 0 && (i == 0 || i == 1)) {
                        if (cmor_has_variable_attribute(var_id, "coordinates")
                            == 0) {
                            cmor_get_variable_attribute(var_id, "coordinates",
                                                        &msg);
                            cmor_cat_unique_string(msg,
                                                   cmor_vars[cmor_grids
                                                             [nGridID].
                                                             associated_variables
                                                             [i]].id);
                        } else {
                            strncpy(msg,
                                    cmor_vars[cmor_grids
                                              [nGridID].associated_variables
                                              [i]].id,
                                    CMOR_MAX_STRING - strlen(msg));
                        }
                        cmor_set_variable_attribute_internal(var_id,
                                                             "coordinates",
                                                             'c', msg);
                        m2[i] = 1;
                    }
                    l++;
                }
            }
/* -------------------------------------------------------------------- */
/*      vertices need to be added                                       */
/* -------------------------------------------------------------------- */

            if (((i == 2) || (i == 3)) && (m == 0)) {
/* -------------------------------------------------------------------- */
/*      ok now it has been defined                                      */
/* -------------------------------------------------------------------- */

                m = 1;
                ierr = nc_def_dim(ncafid, "vertices",
                                  cmor_axes[cmor_vars[j].axes_ids
                                            [cmor_vars[j].ndims - 1]].length,
                                  &nc_dims_associated[l]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF error (%i: %s) while defining\n! "
                        "vertices dimension, variable %s\n! "
                        "(table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                }
            }
            mtype = cmor_vars[j].type;
            ierr = NC_NOERR;
            if (mtype == 'd')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_DOUBLE,
                                  cmor_vars[j].ndims, &nc_dims_associated[0],
                                  &nc_associated_vars[i]);
            else if (mtype == 'f')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_FLOAT,
                                  cmor_vars[j].ndims, &nc_dims_associated[0],
                                  &nc_associated_vars[i]);
            else if (mtype == 'l')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_INT,
                                  cmor_vars[j].ndims, &nc_dims_associated[0],
                                  &nc_associated_vars[i]);
            else if (mtype == 'i')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_INT,
                                  cmor_vars[j].ndims, &nc_dims_associated[0],
                                  &nc_associated_vars[i]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF error (%i: %s) while defining\n! "
                    "associated variable %s, of variable\n! "
                    "%s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr),
                    cmor_vars[j].id, cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            }

/* -------------------------------------------------------------------- */
/*      Creates attributes related to that variable                     */
/* -------------------------------------------------------------------- */
            for (k = 0; k < cmor_vars[j].nattributes; k++) {

/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
                if (cmor_has_variable_attribute(j, cmor_vars[j].attributes[k])
                    != 0) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
                    continue;
                }
                if (strcmp(cmor_vars[j].attributes[k], "flag_values") == 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */
                    ierr =
                      cmor_convert_string_to_list(cmor_vars
                                                  [j].attributes_values_char[k],
                                                  'i', (void *)&int_list,
                                                  &nelts);

                    ierr = nc_put_att_int(ncafid, nc_associated_vars[i],
                                          "flag_values", NC_INT, nelts,
                                          int_list);

                    if (ierr != NC_NOERR) {
                        cmor_handle_error_var_variadic(
                            "NetCDF Error (%i: %s) setting\n! "
                            "flags numerical attribute on\n! "
                            "associated variable %s, for\n! "
                            "variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            ierr, nc_strerror(ierr), cmor_vars[j].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    }
                    free(int_list);
                } else if (cmor_vars[j].attributes_type[k] == 'c') {
                    ierr = cmor_put_nc_char_attribute(ncafid,
                                                      nc_associated_vars[i],
                                                      cmor_vars[j].attributes
                                                      [k],
                                                      cmor_vars
                                                      [j].attributes_values_char
                                                      [k], cmor_vars[j].id);
                } else {
                    ierr = cmor_put_nc_num_attribute(ncafid,
                                                     nc_associated_vars[i],
                                                     cmor_vars[j].attributes[k],
                                                     cmor_vars
                                                     [j].attributes_type[k],
                                                     cmor_vars
                                                     [j].attributes_values_num
                                                     [k], cmor_vars[j].id);
                }
            }
/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */

            if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
                if (cmor_vars[j].ndims > 0) {

                    ics =
                      cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].
                                                                  ref_var_id].
                      shuffle;
                    icd =
                      cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].
                                                                  ref_var_id].
                      deflate;
                    icdl =
                      cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].
                                                                  ref_var_id].
                      deflate_level;
                    icz =
                      cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].
                                                                  ref_var_id].
                      zstandard_level;

                    if (icd != 0) {
                        ierr |= nc_def_var_deflate(ncafid, nc_associated_vars[i],
                                                ics, icd, icdl);
                    } else {
                        ierr |= nc_def_var_deflate(ncafid, nc_associated_vars[i],
                                                ics, 0, 0);
                        ierr |= nc_def_var_zstandard(ncafid, nc_associated_vars[i],
                                                icz);
                    }
                    if (ierr != NC_NOERR) {
                        cmor_handle_error_var_variadic(
                            "NetCDF Error (%i: %s) defining\n! "
                            "compression parameters for\n! "
                            "associated variable '%s' for\n! "
                            "variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            ierr, nc_strerror(ierr), cmor_vars[j].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    }
                }
            }
        }
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                create_singleton_dimensions()                         */
/************************************************************************/
void create_singleton_dimensions(int var_id, int ncid, int *nc_singletons,
                                 int *nc_singletons_bnds, int *dim_bnds)
{
    int ierr;
    int i, j, k;
    char msg[CMOR_MAX_STRING];
    int nVarRefTblID;
    int maxStrLen;

    cmor_add_traceback("create_singleton_dimensions");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

/* -------------------------------------------------------------------- */
/*      find maximum string length for singleton dimension variables    */
/* -------------------------------------------------------------------- */
    maxStrLen = 0;
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        j = cmor_vars[var_id].singleton_ids[i];
        if (j != -1) {
            if (cmor_tables[cmor_axes[j].ref_table_id].axes
                [cmor_axes[j].ref_axis_id].type == 'c') {
                k = strlen(cmor_tables[cmor_axes[j].ref_table_id].axes
                            [cmor_axes[j].ref_axis_id].cvalue);
                if (k > maxStrLen) {
                    maxStrLen = k;
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Creates singleton dimension variables                           */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        j = cmor_vars[var_id].singleton_ids[i];
        if (j != -1) {
            if (cmor_tables[cmor_axes[j].ref_table_id].axes
                [cmor_axes[j].ref_axis_id].type == 'c') {
                if(nc_inq_dimid(ncid, "strlen", &k) != NC_NOERR) {
                    ierr =
                        nc_def_dim(ncid, "strlen", maxStrLen, &k);
                }
                ierr =
                  nc_def_var(ncid, cmor_axes[j].id, NC_CHAR, 1, &k,
                             &nc_singletons[i]);
            } else {
                ierr = nc_def_var(ncid, cmor_axes[j].id, NC_DOUBLE, 0,
                                  &nc_singletons[i], &nc_singletons[i]);
            }
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) defining scalar variable\n! "
                    "%s for variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_axes[j].id,
                    cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            }
/* -------------------------------------------------------------------- */
/*      now  puts on its attributes                                     */
/* -------------------------------------------------------------------- */
            for (k = 0; k < cmor_axes[j].nattributes; k++) {
                if (cmor_axes[j].attributes_type[k] == 'c') {
                    ierr = cmor_put_nc_char_attribute(ncid, nc_singletons[i],
                                                      cmor_axes[j].attributes
                                                      [k],
                                                      cmor_axes
                                                      [j].attributes_values_char
                                                      [k],
                                                      cmor_vars[var_id].id);
                } else {
                    ierr = cmor_put_nc_num_attribute(ncid, nc_singletons[i],
                                                     cmor_axes[j].attributes[k],
                                                     cmor_axes
                                                     [j].attributes_type[k],
                                                     cmor_axes
                                                     [j].attributes_values_num
                                                     [k], cmor_vars[var_id].id);
                }
            }
/* -------------------------------------------------------------------- */
/*      ok we need to see if there's bounds as well...                  */
/* -------------------------------------------------------------------- */

            if (cmor_axes[j].bounds != NULL) {  /*yep */
                snprintf(msg, CMOR_MAX_STRING, "%s_bnds", cmor_axes[j].id);
                ierr = cmor_put_nc_char_attribute(ncid, nc_singletons[i],
                                                  "bounds", msg,
                                                  cmor_vars[var_id].id);
                ierr =
                  nc_def_var(ncid, msg, NC_DOUBLE, 1, dim_bnds,
                             &nc_singletons_bnds[i]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) defining scalar\n! "
                        "bounds variable %s for variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr), cmor_axes[j].id,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                }
            }
        }
    }
    cmor_pop_traceback();

}

/************************************************************************/
/*                       compare_txt_attributes()                       */
/************************************************************************/
int compare_txt_attributes(int ncid, int srcid, int destid, char* name) {
    size_t attlen;
    char *srcattr;
    char *destattr;
    int ret;

    if (nc_inq_attlen(ncid, srcid, name, &attlen)) {
        cmor_handle_error_variadic(
            "cannot determine size of attribute %s", CMOR_CRITICAL, name);
    };
    srcattr = malloc(attlen * sizeof(char));

	if (nc_get_att_text(ncid, srcid, name, srcattr)) {
	    cmor_handle_error_variadic(
            "cannot retrieve value of attribute %s", CMOR_CRITICAL, name);
	};

    if (nc_inq_attlen(ncid, destid, name, &attlen)) {
        cmor_handle_error_variadic(
            "cannot determine size of attribute %s", CMOR_CRITICAL, name);
    };
    destattr = malloc(attlen * sizeof(char));

	if (nc_get_att_text(ncid, destid, name, destattr)) {
	    cmor_handle_error_variadic(
            "cannot retrieve value of attribute %s", CMOR_CRITICAL, name);
	};
    ret = strcmp(srcattr, destattr);
    free(destattr);
    free(srcattr);
    if (ret != 0) {
        cmor_handle_error_variadic(
            "'%s' attribute does not match", CMOR_CRITICAL, name);
    }
    return ret;
}

/************************************************************************/
/*                         copy_txt_attribute()                         */
/************************************************************************/
int copy_txt_attribute(int ncid, int srcid, int destid, char* name, char* suffix) {
    size_t attlen;
    char *srcattr;
    char *destattr;
    int ierr;

    if (nc_inq_attlen(ncid, srcid, name, &attlen)) {
        cmor_handle_error_variadic(
            "cannot determine size of attribute %s", CMOR_CRITICAL, name);
    };
    srcattr = malloc(attlen * sizeof(char));

	if (nc_get_att_text(ncid, srcid, name, srcattr)) {
	    cmor_handle_error_variadic(
            "cannot retrieve value of attribute %s", CMOR_CRITICAL, name);
	};
	if (strcmp(suffix, "") == 0) {
	    destattr = srcattr;
	} else {
	    destattr = malloc(strlen(srcattr) + strlen(suffix) + 1);
        strcpy(destattr, srcattr);
        strcat(destattr, suffix);

	}
    if (nc_put_att_text(ncid, destid, name, strlen(destattr) + 1, destattr)) {
        cmor_handle_error_variadic(
            "cannot copy attribute %s", CMOR_CRITICAL, name);
    }
    if (strcmp(suffix, "") != 0) {
        free(destattr);
    }
    free(srcattr);
    return (0);
}

/************************************************************************/
/*                          set_txt_attribute()                         */
/************************************************************************/
int set_txt_attribute(int ncid, int destid, char* name, char* val){
    if (nc_put_att_text(ncid, destid, name, strlen(val) + 1, val)) {
        cmor_handle_error_variadic(
            "cannot write '%s' to attribute %s", CMOR_CRITICAL, val, name);
    }
    return (0);
}

/************************************************************************/
/*                      calculate_leadtime_coord()                      */
/************************************************************************/
int calculate_leadtime_coord(int var_id) {
    int i = 0;
    int ncid = 0;
    int retval = 0;
    int ierr = 0;
    int leadtime = 0;
    int time_dim = 0;
    int reftime = 0;
    int time = 0;
    size_t timelen;
    double *time_vals;
    double *leadtime_vals;
    double *reftime_val;
    static size_t start[] = {0};
    static size_t count[] = {0};
    extern cmor_dataset_def cmor_current_dataset;
    extern cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];

    cmor_add_traceback("cmor_calculate_leadtime_coord");
    cmor_is_setup();
    ncid = cmor_current_dataset.associated_file;

    /* need both time and reftime for leadtime calculation */
    if (nc_inq_dimid(ncid, "time", &time_dim)) {
        cmor_handle_error_variadic(
            "'time' dimension not present in the file", CMOR_CRITICAL);
    }
    if (nc_inq_dimlen(ncid, time_dim, &timelen)) {
        cmor_handle_error_variadic(
            "cannot determine length of the time dimension", CMOR_CRITICAL);
    }
    if (nc_inq_varid(ncid, "reftime", &reftime)) {
        cmor_handle_error_variadic(
            "'reftime' variable not present in the file", CMOR_CRITICAL);
    }
    if (nc_inq_varid(ncid, "time", &time)) {
        cmor_handle_error_variadic(
            "'time' variable not present in the file", CMOR_CRITICAL);
    }
    if (compare_txt_attributes(ncid, time, reftime, "units") || compare_txt_attributes(ncid, time, reftime, "calendar")) {
        cmor_pop_traceback();
        return(1);
    }

    reftime_val = malloc(sizeof(double));
    time_vals = malloc(timelen * sizeof(double));
    leadtime_vals = malloc(timelen * sizeof(double));

    /* get values for the calculation */

    /* reftime is scalar */
    if (nc_get_var_double(ncid, reftime, reftime_val)) {
        cmor_handle_error_variadic(
            "cannot retrieve value of 'reftime' variable", CMOR_CRITICAL);
    }

    /* update length of the vector for time coord */
    count[0] = timelen;

    if (nc_get_vara_double(ncid, time, start, count, time_vals)) {
        cmor_handle_error_variadic(
            "cannot retrieve values of 'time' variable", CMOR_CRITICAL);
    }
    /* calculate leadtime */
    for (i = 0; i < timelen; i++) {
        leadtime_vals[i] = time_vals[i] - reftime_val[0];
        if (leadtime_vals[i] < 0.0) {
            cmor_handle_error_variadic(
                "'leadtime' for timestep %i is negative", CMOR_CRITICAL, i);
        }
    }

    /* activate define mode */
    nc_redef(ncid);
    /* add leadtime */
    if (nc_inq_varid(ncid, "leadtime", &leadtime)) {
        if (nc_def_var(ncid, "leadtime", NC_DOUBLE, 1, &time_dim, &leadtime)) {
            cmor_handle_error_variadic(
                "cannot add 'leadtime' variable", CMOR_CRITICAL);
        }
    }

    /* variable attributes */
    set_txt_attribute(ncid, leadtime, "axis", "T");
    set_txt_attribute(ncid, leadtime, "units", "days");
    set_txt_attribute(ncid, leadtime, "long_name", "Time elapsed since the start of the forecast");
    set_txt_attribute(ncid, leadtime, "standard_name", "forecast_period");

    /* update coordinates attribute */
    copy_txt_attribute(ncid, cmor_vars[var_id].nc_var_id, cmor_vars[var_id].nc_var_id, "coordinates", " leadtime");

    /* deactivate define mode */
    ierr = nc_enddef(ncid);
    if (ierr != NC_NOERR) {
        cmor_handle_error_var_variadic(
            "NetCDF Error (%i: %s) leaving definition mode",
            CMOR_CRITICAL, var_id,
            ierr, nc_strerror(ierr));
    }
    if (nc_put_vara_double(ncid, leadtime, start, count, leadtime_vals)) {
        cmor_handle_error_variadic(
            "cannot save 'leadtime' coordinates", CMOR_CRITICAL);
    }

    free(leadtime_vals);
    free(time_vals);
    free(reftime_val);
    return (0);
}

/************************************************************************/
/*                             cmor_write()                             */
/************************************************************************/
int cmor_write(int var_id, void *data, char type, char *file_suffix,
               int ntimes_passed, double *time_vals, double *time_bounds,
               int *refvar)
{
    extern cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
    extern cmor_axis_t cmor_axes[CMOR_MAX_AXES];
    extern int cmor_nvars;
    extern cmor_dataset_def cmor_current_dataset;

    int i, ierr = 0, ncid, ncafid;
    char outname[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char appending_to[CMOR_MAX_STRING];
    size_t nc_dim_chunking[CMOR_MAX_AXES];
    int nc_vars[CMOR_MAX_VARIABLES];
    int nc_vars_af[CMOR_MAX_VARIABLES];
    int nc_dim_af[CMOR_MAX_DIMENSIONS];
    int nc_associated_vars[6];
    int nc_bnds_vars[CMOR_MAX_VARIABLES];
    int dim_bnds;
    int nc_singletons[CMOR_MAX_DIMENSIONS];
    int nc_singletons_bnds[CMOR_MAX_DIMENSIONS];
    char mtype;
    int nzfactors = 0;
    int nc_dim[CMOR_MAX_AXES];
    int zfactors[CMOR_MAX_VARIABLES];
    int nc_zfactors[CMOR_MAX_VARIABLES];
    int refvarid;

    int nVarRefTblID;
    char szPathTemplate[CMOR_MAX_STRING];
    char outpath[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_write");

    nVarRefTblID = cmor_vars[var_id].ref_table_id;

    strcpy(appending_to, "");   /* initialize to nothing */
    strcpy(outname, "");
    strcpy(ctmp, "");
    strcpy(msg, "");
    strcpy(ctmp2, "");
    strcpy(outpath, "");

    cmor_is_setup();
    if (var_id > cmor_nvars) {
        cmor_handle_error_variadic("var_id %i not defined", CMOR_CRITICAL, var_id);
        cmor_pop_traceback();
        return (-1);
    };

/* -------------------------------------------------------------------- */
/*    Make sure that time_vals (and possibly time_bounds) are passed    */
/*    when CMOR is running in append mode.                              */
/* -------------------------------------------------------------------- */
    if (bAppendMode) {
        size_t refTableID = cmor_vars[var_id].ref_table_id;
        size_t refAxisID = cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_axis_id;
        if ( time_vals == NULL || 
          ( cmor_tables[refTableID].axes[refAxisID].must_have_bounds == 1 && 
            time_bounds == NULL ) ) {

            cmor_handle_error_variadic(
                "time_vals and time_bounds must be passed through cmor_write "
                "when in append mode", CMOR_CRITICAL);
            cmor_pop_traceback();
            return (-1);
        };
    };

    ierr += cmor_addVersion();
    ierr += cmor_addRIPF(ctmp);

/* -------------------------------------------------------------------- */
/*    Make sure that variable_id is set Global Attributes and for       */
/*    File and Path template                                            */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VARIABLE_ID,
                                            cmor_vars[var_id].id, 1);
    ctmp[0] = '\0';
/* -------------------------------------------------------------------- */
/*      here we check that the variable actually has all                */
/*      the required attributes set                                     */
/* -------------------------------------------------------------------- */
    ierr += cmor_has_required_variable_attributes(var_id);

/* -------------------------------------------------------------------- */
/*  Do we have associated variables (z_factors)?                        */
/* -------------------------------------------------------------------- */
    refvarid = cmor_set_refvar(var_id, refvar, ntimes_passed);

/* -------------------------------------------------------------------- */
/*      Here we check that the types are consistent between             */
/*      the missing value passed and the type passed now                */
/* -------------------------------------------------------------------- */
    cmor_checkMissing(refvarid, var_id, type);

/* -------------------------------------------------------------------- */
/*      Variable never been thru cmor_write,                            */
/*      we need to define everything                                    */
/* -------------------------------------------------------------------- */
    if (cmor_vars[refvarid].initialized == -1) {

        if (cmor_vars[refvarid].type != type) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Converted type from '%c' to '%c'", type,
                     cmor_vars[refvarid].type);
            cmor_update_history(refvarid, msg);
        }

        ierr += cmor_setGblAttr(var_id);

/* -------------------------------------------------------------------- */
/*      Figures out path                                                */
/* -------------------------------------------------------------------- */
        strncpy(szPathTemplate, cmor_current_dataset.path_template,
                CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*     Add outpath prefix if exist.                                     */
/* -------------------------------------------------------------------- */
        strncpytrim(outname, cmor_current_dataset.outpath, CMOR_MAX_STRING);
/* -------------------------------------------------------------------- */
/*     Make sure last character is '/'.                                 */
/* -------------------------------------------------------------------- */
        if ((strlen(outname) > 0) && (outname[strlen(outname)] != '/')) {
            strncat(outname, "/", CMOR_MAX_STRING);
        }

        if (CMOR_CREATE_SUBDIRECTORIES == 1) {
            ierr +=
              cmor_CreateFromTemplate(nVarRefTblID, szPathTemplate, outname,
                                      "/");
        } else {
            ierr +=
              cmor_CreateFromTemplate(nVarRefTblID, szPathTemplate, msg, "/");
        }

        if (ierr != 0) {
            cmor_handle_error_var_variadic(
                "Cannot continue until you fix the errors listed above: %d",
                CMOR_CRITICAL, var_id, ierr);
            cmor_pop_traceback();
            return (1);
        }

        ierr = cmor_mkdir(outname);
        if ((ierr != 0) && (errno != EEXIST)) {
            cmor_handle_error_var_variadic(
                "creating outpath: %s, for variable %s (table: %s). "
                "Not enough permission?",
                CMOR_CRITICAL, var_id,
                outname, cmor_vars[var_id].id,
                cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);

        }

        strncat(outname, "/", CMOR_MAX_STRING - strlen(outname));
/* -------------------------------------------------------------------- */
/*    Verify that var name does not contain "_" or "-"                  */
/* -------------------------------------------------------------------- */
        for (i = 0; i < strlen(cmor_vars[var_id].id); i++) {
            if ((cmor_vars[var_id].id[i] == '_') ||
                (cmor_vars[var_id].id[i] == '-')) {
                cmor_handle_error_var_variadic(
                    "var_id cannot contain %c you passed: %s "
                    "(table: %s). Please check your input tables\n! ",
                    CMOR_CRITICAL, var_id,
                    cmor_vars[var_id].id[i], cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
                cmor_pop_traceback();
                return (1);

            }
        }

/* -------------------------------------------------------------------- */
/*    Create/Save filename                                              */
/* -------------------------------------------------------------------- */
        ierr = cmor_CreateFromTemplate(nVarRefTblID,
                                       cmor_current_dataset.file_template,
                                       outname, "_");

        strcat(outpath, outname);
        strncpy(outname, outpath, CMOR_MAX_STRING);
        strncpytrim(cmor_vars[var_id].base_path, outname, CMOR_MAX_STRING);
        strcat(outname, "XXXXXX");
        ierr = mkstemp(outname);
        unlink(outname);
/* -------------------------------------------------------------------- */
/*      Add Process ID and a random number to filename                  */
/* -------------------------------------------------------------------- */
        sprintf(msg, "%d", (int)getpid());
        strncat(outname, msg, CMOR_MAX_STRING - strlen(outname));

/* -------------------------------------------------------------------- */
/*      Add the '.nc' extension                                         */
/* -------------------------------------------------------------------- */
        strncat(outname, ".nc", CMOR_MAX_STRING - strlen(outname));
        strncpytrim(cmor_vars[var_id].current_path, outname, CMOR_MAX_STRING);
/* -------------------------------------------------------------------- */
/*      Decides NetCDF mode                                             */
/* -------------------------------------------------------------------- */
        ncid = cmor_validateFilename(outname, file_suffix, var_id);
        cmor_current_dataset.associated_file = ncid;
        if(!bAppendMode) {
/* -------------------------------------------------------------------- */
/*      we closed and reopened the same test, in case we                */
/*      were appending, in which case all declaration have              */
/*      been done if the open loop                                      */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      Variable never been thru cmor_write, we need to                 */
/*      define everything                                               */
/*                                                                      */
/*      define global attributes                                        */
/* -------------------------------------------------------------------- */
            if (cmor_current_dataset.initiated == 0) {
                cmor_handle_error_var_variadic(
                    "you need to initialize the dataset by calling "
                    "cmor_dataset_json before calling cmor_write",
                    CMOR_NORMAL, var_id);
                cmor_pop_traceback();
                return (1);
            }

            cleanup_varid = var_id;
            ncafid = ncid;

/* -------------------------------------------------------------------- */
/*      make sure we are in def mode                                    */
/* -------------------------------------------------------------------- */
            ierr = nc_redef(ncafid);
            if (ierr != NC_NOERR && ierr != NC_EINDEFINE) {
                cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) putting metadata file (%s) in\n! "
                        "def mode, nc file id was: %i, you were writing\n! "
                        "variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_current_dataset.associated_file_name, ncafid,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_pop_traceback();
                return (1);
            }

            ierr = cmor_writeGblAttr(var_id, ncid, ncafid);
            if (ierr != 0) {
                return (ierr);
            }

/* -------------------------------------------------------------------- */
/*      store netcdf file id associated with this variable              */
/* -------------------------------------------------------------------- */
            cmor_vars[var_id].initialized = ncid;

/* -------------------------------------------------------------------- */
/*      define dimensions in NetCDF file                                */
/* -------------------------------------------------------------------- */
            cmor_define_dimensions(var_id, ncid, ncafid, time_bounds, nc_dim,
                    nc_vars, nc_bnds_vars, nc_vars_af, nc_dim_chunking,
                    &dim_bnds, zfactors, nc_zfactors, nc_dim_af, &nzfactors);
/* -------------------------------------------------------------------- */
/*      Store the dimension id for reuse when writing                   */
/*      over multiple call to cmor_write                                */
/* -------------------------------------------------------------------- */
            cmor_vars[var_id].time_nc_id = nc_vars[0];

            int nGridID;
            nGridID = cmor_vars[var_id].grid_id;
/* -------------------------------------------------------------------- */
/*      check if it is a grid thing                                     */
/* -------------------------------------------------------------------- */
            if (nGridID > -1) {
                ierr = cmor_grids_def(var_id, nGridID, ncafid, nc_dim_af,
                        nc_associated_vars);
                if (ierr)
                    return (ierr);
            }

            create_singleton_dimensions(var_id, ncid, nc_singletons,
                    nc_singletons_bnds, &dim_bnds);

/* -------------------------------------------------------------------- */
/*      Creating variable to write                                      */
/* -------------------------------------------------------------------- */
            mtype = cmor_vars[var_id].type;
            if (mtype == 'd')
                ierr = nc_def_var(ncid, cmor_vars[var_id].id, NC_DOUBLE,
                        cmor_vars[var_id].ndims, &nc_dim[0],
                        &nc_vars[cmor_vars[var_id].ndims]);
            else if (mtype == 'f')
                ierr = nc_def_var(ncid, cmor_vars[var_id].id, NC_FLOAT,
                        cmor_vars[var_id].ndims, &nc_dim[0],
                        &nc_vars[cmor_vars[var_id].ndims]);
            else if (mtype == 'l')
                ierr = nc_def_var(ncid, cmor_vars[var_id].id, NC_INT,
                        cmor_vars[var_id].ndims, &nc_dim[0],
                        &nc_vars[cmor_vars[var_id].ndims]);
            else if (mtype == 'i')
                ierr = nc_def_var(ncid, cmor_vars[var_id].id, NC_INT,
                        cmor_vars[var_id].ndims, &nc_dim[0],
                        &nc_vars[cmor_vars[var_id].ndims]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) writing variable: %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            }

/* -------------------------------------------------------------------- */
/*      Store the var id for reuse when writing                         */
/*      over multiple call to cmor_write and for cmor_close             */
/* -------------------------------------------------------------------- */
            cmor_vars[var_id].nc_var_id = nc_vars[cmor_vars[var_id].ndims];

            cmor_create_var_attributes(var_id, ncid, ncafid, nc_vars,
                    nc_bnds_vars, nc_vars_af, nc_associated_vars, nc_singletons,
                    nc_singletons_bnds, nc_zfactors, zfactors, nzfactors,
                    nc_dim_chunking, outname);


        }

    } else {
/* --------------------------------------------------------------------- */
/*      Variable already been thru cmor_write,                          */
/*      we just get the netcdf file id                                  */
/* -------------------------------------------------------------------- */
        ncid = cmor_vars[refvarid].initialized;

/* -------------------------------------------------------------------- */
/*      generates a new unique id                                       */
/* -------------------------------------------------------------------- */
        cmor_generate_uuid();
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_TRACKING_ID, ctmp2);
        ierr = nc_put_att_text(ncid, NC_GLOBAL, GLOBAL_ATT_TRACKING_ID,
                               strlen(ctmp2), ctmp2);
        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "NetCDF error (%i: %s) for variable %s (table: %s)\n! "
                "writing global attribute: %s (%s)",
                CMOR_CRITICAL, var_id,
                ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id,
                "tracking_id", (char *)ctmp2);
            cmor_pop_traceback();
            return (1);

        }

/* -------------------------------------------------------------------- */
/*      in case we are doing a zfactor var                              */
/* -------------------------------------------------------------------- */
        cmor_vars[var_id].time_nc_id = cmor_vars[refvarid].time_nc_id;
        cmor_vars[var_id].time_bnds_nc_id = cmor_vars[refvarid].time_bnds_nc_id;

    }

/* -------------------------------------------------------------------- */
/*      here we add the number of time                                  */
/*      written for the associated variable                             */
/* -------------------------------------------------------------------- */
    if ((refvar != NULL) && (cmor_vars[refvarid].grid_id > -1)
        && (cmor_grids[cmor_vars[refvarid].grid_id].istimevarying == 1)) {
        for (i = 0; i < 4; i++) {
            if (cmor_grids[cmor_vars[refvarid].grid_id].associated_variables
                [i] == var_id) {
                if (cmor_vars[refvarid].ntimes_written_coords[i] == -1) {
                    cmor_vars[refvarid].ntimes_written_coords[i] =
                      ntimes_passed;
                } else {
                    cmor_vars[refvarid].ntimes_written_coords[i] +=
                      ntimes_passed;
                }
            }
        }
    }
    if (refvar != NULL) {
        for (i = 0; i < 10; i++) {
            if (cmor_vars[*refvar].associated_ids[i] == var_id) {
                if (cmor_vars[*refvar].ntimes_written_associated[i] == 0) {
                    cmor_vars[*refvar].ntimes_written_associated[i] =
                      ntimes_passed;
                } else {
                    cmor_vars[*refvar].ntimes_written_associated[i] +=
                      ntimes_passed;
                }
            }
        }
    }
    if(bAppendMode && refvar != NULL) {
      size_t starts[2];
      if ( cmor_vars[var_id].last_time == -999 ) {
            starts[0] = cmor_vars[refvarid].ntimes_written - 2;
            ierr = nc_get_var1_double(ncid, cmor_vars[var_id].time_nc_id,
                                      &starts[0], &cmor_vars[var_id].last_time);
      } 
      if( cmor_vars[var_id].last_bound == 1.e20 ) {
                starts[0] = cmor_vars[refvarid].ntimes_written - 2;
                starts[1] = 1;
                ierr = nc_get_var1_double(ncid,
                                          cmor_vars[var_id].time_bnds_nc_id,
                                          &starts[0],
                                          &cmor_vars[var_id].last_bound);
      }
    }
    cmor_write_var_to_file(ncid, &cmor_vars[var_id], data, type,
                           ntimes_passed, time_vals, time_bounds);
/* -------------------------------------------------------------------- */
/*      Check if we need to add leadtime coordinate                     */
/* -------------------------------------------------------------------- */
    if (strstr(cmor_tables[cmor_vars[var_id].ref_table_id].vars[cmor_vars[var_id].ref_var_id].dimensions_str, AXIS_FORECAST_LEADTIME)) {
        calculate_leadtime_coord(var_id);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                 cmor_create_var_attributes()                         */
/************************************************************************/
void cmor_create_var_attributes(int var_id, int ncid, int ncafid,
                                int *nc_vars, int *nc_bnds_vars,
                                int *nc_vars_af, int *nc_associated_vars,
                                int *nc_singletons, int *nc_singletons_bnds,
                                int *nc_zfactors, int *zfactors, int nzfactors,
                                size_t * nc_dim_chunking, char *outname)
{

    size_t starts[2], counts[2];
    int i, j, k, l, ho;
    int ierr;
    char msg[CMOR_MAX_STRING];
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int nelts;
    int *int_list = NULL;
    int ics, icd, icdl, icz, icqm, icqn;
    int bChunk;
    cmor_add_traceback("cmor_create_var_attributes");
/* -------------------------------------------------------------------- */
/*      Creates attributes related to that variable                     */
/* -------------------------------------------------------------------- */
    for (j = 0; j < cmor_vars[var_id].nattributes; j++) {
/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
        if (cmor_has_variable_attribute(var_id,
                                        cmor_vars[var_id].attributes[j]) != 0) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
            continue;
        }
        if (strcmp(cmor_vars[var_id].attributes[j], "flag_values") == 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */

            ierr =
              cmor_convert_string_to_list(cmor_vars
                                          [var_id].attributes_values_char[j],
                                          'i', (void *)&int_list, &nelts);
            ierr =
              nc_put_att_int(ncid, cmor_vars[var_id].nc_var_id, "flag_values",
                             NC_INT, nelts, int_list);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) setting flags numerical\n! "
                    "attribute on variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
                cmor_pop_traceback();
                return;

            }
            free(int_list);

        } else if (cmor_vars[var_id].attributes_type[j] == 'c') {
            ierr = cmor_put_nc_char_attribute(ncid,
                                              cmor_vars[var_id].nc_var_id,
                                              cmor_vars[var_id].attributes[j],
                                              cmor_vars
                                              [var_id].attributes_values_char
                                              [j], cmor_vars[var_id].id);
        } else {
            ierr = cmor_put_nc_num_attribute(ncid,
                                             cmor_vars[var_id].nc_var_id,
                                             cmor_vars[var_id].attributes[j],
                                             cmor_vars[var_id].attributes_type
                                             [j],
                                             cmor_vars
                                             [var_id].attributes_values_num[j],
                                             cmor_vars[var_id].id);
        }
    }

    if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
        && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
        && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
        cmor_var_t *pVar;

        pVar = &cmor_vars[var_id];

        ics = pVar->shuffle;
        icd = pVar->deflate;
        icdl = pVar->deflate_level;
        icz = pVar->zstandard_level;
        icqm = pVar->quantize_mode;
        icqn = pVar->quantize_nsd;
        ierr = nc_def_var_quantize(ncid, pVar->nc_var_id, icqm, icqn);

        // Only use zstandard compression if deflate is disabled
        if (icd != 0) {
            ierr |= nc_def_var_deflate(ncid, pVar->nc_var_id, ics, icd, icdl);
        } else {
            ierr |= nc_def_var_deflate(ncid, pVar->nc_var_id, ics, 0, 0);
            ierr |= nc_def_var_zstandard(ncid, pVar->nc_var_id, icz);
        }

        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "NetCDF Error (%i: %s) defining compression\n! "
                "parameters for variable '%s' (table: %s)",
                CMOR_CRITICAL, var_id,
                ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id);
            cmor_pop_traceback();
            return;

        }
/* -------------------------------------------------------------------- */
/*      Chunking stuff                                                  */
/* -------------------------------------------------------------------- */

#ifndef NC_CHUNKED
#define NC_CHUNKED 0
#endif
        size_t nc_dim_chunking[cmor_vars[var_id].ndims];
        bChunk = cmor_set_chunking(var_id, nVarRefTblID, nc_dim_chunking);
        if (bChunk != -1 && (!((cmor_vars[var_id].grid_id > -1)
                               &&
                               (cmor_grids
                                [cmor_vars[var_id].grid_id].istimevarying ==
                                1)))) {
            ierr =
              nc_def_var_chunking(ncid, cmor_vars[var_id].nc_var_id, NC_CHUNKED,
                                 NULL);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDFTestTables/CMIP6_chunking.json: Error (%i: %s) defining chunking\n! "
                    "parameters for variable '%s' (table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
                cmor_pop_traceback();
                return;

            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Done with NetCDF file definitions                               */
/* -------------------------------------------------------------------- */

    ierr = nc_enddef(ncid);
    if (ierr != NC_NOERR && ierr != NC_ENOTINDEFINE) {
        cmor_handle_error_var_variadic(
            "NetCDF Error (%i: %s) leaving definition mode for file %s",
            CMOR_CRITICAL, var_id,
            ierr, nc_strerror(ierr), outname);
        cmor_pop_traceback();
        return;

    }
    ierr = nc_enddef(ncafid);
    if (ierr != NC_NOERR && ierr != NC_ENOTINDEFINE) {
        cmor_handle_error_var_variadic(
            "NetCDF Error (%i: %s) leaving definition mode for metafile %s",
            CMOR_CRITICAL, var_id,
            ierr, nc_strerror(ierr),
            cmor_current_dataset.associated_file_name);
        cmor_pop_traceback();
        return;

    }

/* -------------------------------------------------------------------- */
/*      Write non time dimension of variable into the NetCDF file       */
/* -------------------------------------------------------------------- */
    l = 1;
    if (cmor_axes[cmor_vars[var_id].axes_ids[0]].axis != 'T') {
        l = 0;
    }
    for (i = l; i < cmor_vars[var_id].ndims; i++) {
/* -------------------------------------------------------------------- */
/*      at this point we need to check if the values of the             */
/*      axis need to be replaced (hybrid coords                         */
/*      we only need to do this if ho != hi                             */
/* -------------------------------------------------------------------- */
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_out
            != cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_in) {
            ho = 0;
            if (cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_out != 0) {
                ho = cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_out;
            } else if (cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_in != 0) {
                ho = cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_in;
            }
            if (ho != 0) {
/* -------------------------------------------------------------------- */
/*      yep need to change them                                         */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      std hyb sigma                                                   */
/* -------------------------------------------------------------------- */
                if (ho == 1) {
/* -------------------------------------------------------------------- */
/*      Look for a coeff                                                */
/* -------------------------------------------------------------------- */
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "a") == 0)
                            && (cmor_vars[j].zaxis
                                == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        cmor_handle_error_var_variadic(
                            "could not find 'a' coeff for axis: %s,\n! "
                            "for variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                    for (j = 0;
                         j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                         j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] =
                          cmor_vars[k].values[j];
/* -------------------------------------------------------------------- */
/*      look for b coeff                                                */
/* -------------------------------------------------------------------- */

                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "b") == 0)
                            && (cmor_vars[j].zaxis
                                == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        cmor_handle_error_var_variadic(
                            "could find 'b' coeff for axis: %s,\n! "
                            "for variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                    for (j = 0;
                         j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                         j++) {
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] +=
                          cmor_vars[k].values[j];
                    }
/* -------------------------------------------------------------------- */
/*      do we have bounds to treat as well?                             */
/* -------------------------------------------------------------------- */

                    if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "a_bnds") == 0)
                                && (cmor_vars[j].zaxis
                                    == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            cmor_handle_error_var_variadic(
                                "could not find 'a_bnds' coeff for\n! "
                                "axis: %s, for variable %s (table: %s)",
                                CMOR_CRITICAL, var_id,
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].
                                id, cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                            cmor_pop_traceback();
                            return;

                        }
                        for (j = 0;
                             j
                             < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                             * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] =
                              cmor_vars[k].values[j];
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "b_bnds") == 0)
                                && (cmor_vars[j].zaxis
                                    == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            cmor_handle_error_var_variadic(
                                "could find 'b_bnds' coef for axis:\n! "
                                " %s, for variable %s (table: %s)",
                                CMOR_CRITICAL, var_id,
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                            cmor_pop_traceback();
                            return;

                        }
                        for (j = 0;
                             j
                             < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                             * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].
                              bounds[j] += cmor_vars[k].values[j];
                    }
                    cmor_flip_hybrid(var_id, i, "a", "b", "a_bnds", "b_bnds");
/* -------------------------------------------------------------------- */
/*      alternate hyb sigma                                             */
/* -------------------------------------------------------------------- */
                } else if (ho == 2) {
/* -------------------------------------------------------------------- */
/*      look for ap coeff                                               */
/* -------------------------------------------------------------------- */
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "ap") == 0)
                            && (cmor_vars[j].zaxis
                                == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        cmor_handle_error_var_variadic(
                            "could not find 'ap' coeff for axis:\n! "
                            "%s, for variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                    for (j = 0;
                         j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                         j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] =
                          cmor_vars[k].values[j] / cmor_vars[l].values[0];
/* -------------------------------------------------------------------- */
/*      look for b coeff                                                */
/* -------------------------------------------------------------------- */
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "b") == 0)
                            && (cmor_vars[j].zaxis
                                == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        cmor_handle_error_var_variadic(
                            "could find 'b' coef for axis: %s,\n! "
                            "for variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                    for (j = 0;
                         j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                         j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] +=
                          cmor_vars[k].values[j];

/* -------------------------------------------------------------------- */
/*      deals with bounds                                               */
/* -------------------------------------------------------------------- */

                    if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "ap_bnds") == 0)
                                && (cmor_vars[j].zaxis
                                    == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            cmor_handle_error_var_variadic(
                                "could not find 'ap_bnds' coeff for\n! "
                                "axis: %s, for variable %s\n! "
                                "(table: %s)",
                                CMOR_CRITICAL, var_id,
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].
                                id, cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                            cmor_pop_traceback();
                            return;

                        }
                        for (j = 0;
                             j
                             < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                             * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] =
                              cmor_vars[k].values[j]
                              / cmor_vars[l].values[0];
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "b_bnds") == 0)
                                && (cmor_vars[j].zaxis
                                    == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            cmor_handle_error_var_variadic(
                                "could find 'b_bnds' coef for axis:\n! "
                                "%s, for variable %s (table: %s)",
                                CMOR_CRITICAL, var_id,
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].
                                id, cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                            cmor_pop_traceback();
                            return;

                        }
                        for (j = 0;
                             j
                             < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                             * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].
                              bounds[j] += cmor_vars[k].values[j];
                    }
                    cmor_flip_hybrid(var_id, i, "ap", "b", "ap_bnds", "b_bnds");
/* -------------------------------------------------------------------- */
/*      sigma                                                           */
/* -------------------------------------------------------------------- */
                } else if (ho == 3) {
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "sigma") == 0)
                            && (cmor_vars[j].zaxis
                                == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        cmor_handle_error_var_variadic(
                            "could not find 'sigma' coeff for axis:\n! "
                            "%s, for variable %s (table: %s)",
                            CMOR_CRITICAL, var_id,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                    for (j = 0;
                         j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                         j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] =
                          cmor_vars[k].values[j];
/* -------------------------------------------------------------------- */
/*      deals with bounds                                               */
/* -------------------------------------------------------------------- */

                    if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "sigma_bnds") == 0)
                                && (cmor_vars[j].zaxis
                                    == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            cmor_handle_error_var_variadic(
                                "could not find 'sigma_bnds' coeff\n! "
                                "for axis: %s, for variable %s (table: %s)",
                                CMOR_CRITICAL, var_id,
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                            cmor_pop_traceback();
                            return;

                        }
                        for (j = 0;
                             j
                             < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                             * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] =
                              cmor_vars[k].values[j];
                    }
                }
                cmor_flip_hybrid(var_id, i, "sigma", NULL, "sigma_bnds", NULL);
            }
        }
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues == NULL) {
            if (cmor_axes[cmor_vars[var_id].axes_ids[i]].store_in_netcdf == 1) {
                ierr = nc_put_var_double(ncid, nc_vars[i],
                                         cmor_axes[cmor_vars[var_id].axes_ids
                                                   [i]].values);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) writing axis '%s'\n! "
                        "values for variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                    cmor_pop_traceback();
                    return;

                }
                if (ncid != ncafid) {
                    ierr = nc_put_var_double(ncafid, nc_vars_af[i],
                                             cmor_axes[cmor_vars
                                                       [var_id].
                                                       axes_ids[i]].values);
                    if (ierr != NC_NOERR) {
                        cmor_handle_error_var_variadic(
                            "NetCDF Error (%i: %s) writing axis '%s'\n! "
                            "values to metafile, for variable %s "
                            "(table: %s)",
                            CMOR_CRITICAL, var_id,
                            ierr, nc_strerror(ierr),
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                }
            }
        } else {
            for (j = 0; j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                 j++) {
                starts[0] = j;
                starts[1] = 0;
                counts[0] = 1;
                counts[1] =
                  strlen(cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues[j]);
                ierr =
                  nc_put_vara_text(ncid, nc_vars[i], starts, counts,
                                   cmor_axes[cmor_vars[var_id].axes_ids[i]].
                                   cvalues[j]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) writing axis '%s'\n! "
                        "value number %d (%s), for variable %s "
                        "(table: %s)",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].id, j,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].
                        cvalues[j], cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                    cmor_pop_traceback();
                    return;

                }
                if (ncid != ncafid) {
                    ierr =
                      nc_put_vara_text(ncafid, nc_vars_af[i], starts,
                                       counts,
                                       cmor_axes[cmor_vars[var_id].axes_ids[i]].
                                       cvalues[j]);
                    if (ierr != NC_NOERR) {
                        cmor_handle_error_var_variadic(
                            "NetCDF Error (%i: %s) writing axis '%s'\n! "
                            "values to metafile, for variable %s\n! "
                            "(table: %s)",
                            CMOR_CRITICAL, var_id,
                            ierr, nc_strerror(ierr),
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                        cmor_pop_traceback();
                        return;

                    }
                }
            }
        }
/* -------------------------------------------------------------------- */
/*      ok do we have bounds on this axis?                              */
/* -------------------------------------------------------------------- */
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
            ierr = nc_put_var_double(ncafid, nc_bnds_vars[i],
                                     cmor_axes[cmor_vars[var_id].axes_ids[i]].
                                     bounds);
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NC error (%i: %s) on variable %s writing\n! "
                    "bounds for dim %i (%s), for variable %s\n! "
                    "(table: %s)",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr),
                    cmor_vars[var_id].id, i,
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                    cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
                cmor_pop_traceback();
                return;

            }
        }
    }

/* -------------------------------------------------------------------- */
/*      ok now need to write grid variables                             */
/* -------------------------------------------------------------------- */
    if (cmor_vars[var_id].grid_id > -1) {
        if (cmor_grids[cmor_vars[var_id].grid_id].istimevarying == 0) {
            for (i = 0; i < 4; i++) {
                j =
                  cmor_grids[cmor_vars[var_id].grid_id].associated_variables[i];
                if (j != -1) {  /* we need to write this variable */
                    cmor_vars[j].nc_var_id = nc_associated_vars[i];
                    switch (i) {
                      case (0):
                          cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                                 cmor_grids[cmor_vars
                                                            [var_id].
                                                            grid_id].lats, 'd',
                                                 0, NULL, NULL);
                          break;
                      case (1):
                          cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                                 cmor_grids[cmor_vars
                                                            [var_id].
                                                            grid_id].lons, 'd',
                                                 0, NULL, NULL);
                          break;
                      case (2):
                          cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                                 cmor_grids[cmor_vars
                                                            [var_id].
                                                            grid_id].blats, 'd',
                                                 0, NULL, NULL);
                          break;
                      case (3):
                          cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                                 cmor_grids[cmor_vars
                                                            [var_id].
                                                            grid_id].blons, 'd',
                                                 0, NULL, NULL);
                          break;
                      default:
                          break;
                    }
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      ok now write the zfactor values if necessary                    */
/* -------------------------------------------------------------------- */

    for (i = 0; i < nzfactors; i++) {
/* -------------------------------------------------------------------- */
/*      ok this one has value defined we need to store it               */
/* -------------------------------------------------------------------- */
        if (cmor_vars[zfactors[i]].values != NULL) {
            cmor_vars[zfactors[i]].nc_var_id = nc_zfactors[i];
            cmor_write_var_to_file(ncafid, &cmor_vars[zfactors[i]],
                                   cmor_vars[zfactors[i]].values, 'd', 0, NULL,
                                   NULL);
        }
    }
/* -------------------------------------------------------------------- */
/*      Write singleton dimension variables                             */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        int nRefAxTableID;
        int nRefAxisID;

        j = cmor_vars[var_id].singleton_ids[i];
        nRefAxTableID = cmor_axes[j].ref_table_id;
        nRefAxisID = cmor_axes[j].ref_axis_id;

        if (j != -1) {
            if (cmor_tables[nRefAxTableID].axes[nRefAxisID].type == 'c') {
                ierr = nc_put_var_text(ncid, nc_singletons[i],
                                       cmor_tables[nRefAxTableID].axes
                                       [nRefAxisID].cvalue);
            } else {
                ierr = nc_put_var_double(ncid, nc_singletons[i],
                                         cmor_axes[j].values);
            }
            if (ierr != NC_NOERR) {
                cmor_handle_error_var_variadic(
                    "NetCDF Error (%i: %s) writing scalar variable\n! "
                    "%s for variable %s (table: %s), value: %lf",
                    CMOR_CRITICAL, var_id,
                    ierr, nc_strerror(ierr), cmor_axes[j].id,
                    cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    cmor_axes[j].values[0]);
                cmor_pop_traceback();
                return;

            }
/* -------------------------------------------------------------------- */
/*      now see if we need bounds                                       */
/* -------------------------------------------------------------------- */
            if (cmor_axes[j].bounds != NULL) {
                ierr = nc_put_var_double(ncid, nc_singletons_bnds[i],
                                         cmor_axes[j].bounds);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_var_variadic(
                        "NetCDF Error (%i: %s) writing scalar bounds\n! "
                        "variable %s for variable %s (table: %s),\n! "
                        "values: %lf, %lf",
                        CMOR_CRITICAL, var_id,
                        ierr, nc_strerror(ierr),
                        cmor_axes[j].id, cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id,
                        cmor_axes[j].bounds[0], cmor_axes[j].bounds[1]);
                    cmor_pop_traceback();
                    return;

                }
            }
        }
    }
    cmor_current_dataset.associate_file = ncafid;
    cmor_pop_traceback();
}

/************************************************************************/
/*                    cmor_CreateFromTemplate()                         */
/************************************************************************/
int cmor_CreateFromTemplate(int nVarRefTblID, char *templateSTH,
                            char *szJoin, char *separator)
{
    char *szToken;
    char *szFirstItem;
    char tmp[CMOR_MAX_STRING];
    char path_template[CMOR_MAX_STRING];
    cmor_table_t *pTable;
    int rc;

    pTable = &cmor_tables[nVarRefTblID];

    cmor_add_traceback("cmor_CreateFromTemplate");
    cmor_is_setup();

    strcpy(path_template, templateSTH);
/* -------------------------------------------------------------------- */
/*    Get rid of <> characters from template and add "information"      */
/*    to path                                                           */
/* -------------------------------------------------------------------- */
    szToken = strtok(path_template, GLOBAL_SEPARATORS);
    int optional = 0;
    while (szToken) {

/* -------------------------------------------------------------------- */
/*      Is this an optional token.                                      */
/* -------------------------------------------------------------------- */
        if (strncmp(szToken, GLOBAL_OPENOPTIONAL, 1) == 0) {
            optional = 1;

        } else if (strncmp(szToken, GLOBAL_CLOSEOPTIONAL, 1) == 0) {
            optional = 0;
/* -------------------------------------------------------------------- */
/*      This token must be a global attribute, a table header attribute */
/*      or an internal attribute.  Otherwise we just skip it and the    */
/*      user get a warning.                                             */
/* -------------------------------------------------------------------- */
        } else if (strcmp(szToken, GLOBAL_ATT_CONVENTIONS) == 0) {
            cmor_get_cur_dataset_attribute(szToken, tmp);
            strncat(szJoin, tmp, CMOR_MAX_STRING);
            strcat(szJoin, separator);

        } else if (cmor_has_cur_dataset_attribute(szToken) == 0) {
            cmor_get_cur_dataset_attribute(szToken, tmp);
            szFirstItem = strstr(tmp, " ");
            if (szFirstItem != NULL) {
/* -------------------------------------------------------------------- */
/*  Copy only the first characters before " " for multiple words token  */
/* -------------------------------------------------------------------- */
                strncat(szJoin, tmp, szFirstItem - tmp);
            } else {
                strncat(szJoin, tmp, CMOR_MAX_STRING);
            }
            strcat(szJoin, separator);

        } else if (cmor_get_table_attr(szToken, pTable, tmp) == 0) {
            strncat(szJoin, tmp, CMOR_MAX_STRING);
            strcat(szJoin, separator);

        } else if (strcmp(szToken, OUTPUT_TEMPLATE_RIPF) == 0) {
            rc = cmor_addRIPF(szJoin);
            if (!rc) {
                return (rc);
            }
            strcat(szJoin, separator);
        } else if (strcmp(szToken, GLOBAL_ATT_VARIABLE_ID) == 0) {
            strncat(szJoin, szToken, CMOR_MAX_STRING);
            strcat(szJoin, separator);
            // check if attribute start with '_"
        } else {
            char szInternalAtt[CMOR_MAX_STRING];
            strcpy(szInternalAtt, GLOBAL_INTERNAL);
            strncat(szInternalAtt, szToken,
                CMOR_MAX_STRING - strlen(szInternalAtt));
            if (cmor_has_cur_dataset_attribute(szInternalAtt) == 0) {
                cmor_get_cur_dataset_attribute(szInternalAtt, tmp);
                if (!optional) {
                    strncat(szJoin, tmp, CMOR_MAX_STRING);
                    strcat(szJoin, separator);
                } else {
/* -------------------------------------------------------------------- */
/*      Skip "no-driver for filename if optional is set to 1            */
/* -------------------------------------------------------------------- */
                    if (strcmp(tmp, GLOBAL_ATT_VAL_NODRIVER) != 0) {
                        strncat(szJoin, tmp, CMOR_MAX_STRING);
                        strcat(szJoin, separator);
                    }
                }
/* -------------------------------------------------------------------- */
/*      Just Copy the token without a separator                         */
/* -------------------------------------------------------------------- */
            } else {
                strncat(szJoin, szToken, CMOR_MAX_STRING);
            }
        }

        szToken = strtok(NULL, "><");
    }
/* -------------------------------------------------------------------- */
/*     If the last character is the separator delete it.                */
/* -------------------------------------------------------------------- */
    if (strcmp(&szJoin[strlen(szJoin) - 1], separator) == 0) {
        szJoin[strlen(szJoin) - 1] = '\0';
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                          cmor_addVersion()                           */
/************************************************************************/
int cmor_addVersion()
{
    time_t t;
    struct tm *tm;
    char szVersion[CMOR_MAX_STRING];;
    char szDate[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_addVersion");
    cmor_is_setup();

    time(&t);
    tm = localtime(&t);
    strcpy(szVersion, "v");
    strftime(szDate, CMOR_MAX_STRING, "%Y%m%d", tm);
    strcat(szVersion, szDate);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VERSION, szVersion, 1);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                          cmor_addRIPF()                              */
/************************************************************************/
int cmor_addRIPF(char *variant)
{
    char tmp[CMOR_MAX_STRING];
    int realization_index;
    int initialization_index;
    int physics_index;
    int forcing_index;
    int reti;
    regex_t regex;
    int ierr = 0;
    char szValue[CMOR_MAX_STRING];
    char szVariant[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_addRipf");
    cmor_is_setup();
    reti = regcomp(&regex, "^[[:digit:]]\\{1,\\}$", 0);

/* -------------------------------------------------------------------- */
/*      realization                                                     */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_REALIZATION) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_REALIZATION, tmp);
        if (strlen(tmp) > 4) {
            cmor_handle_error_variadic(
                "Your realization_index \"%s\" is invalid. \n! "
                "It cannot contains more than 4 digits. \n! ",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        reti = regexec(&regex, tmp, 0, NULL, 0);
        if (reti) {
            cmor_handle_error_variadic(
                "Your realization_index \"%s\" is invalid. \n! "
                "It must contain only characters between 0 and 9 \n!",
                CMOR_NORMAL, tmp);
            ierr += -1;
        }

        sscanf(tmp, "%d", &realization_index);
        snprintf(tmp, CMOR_MAX_STRING, "r%d", realization_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }
/* -------------------------------------------------------------------- */
/*      initialization id (re== 0quired)                                */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_INITIA_IDX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_INITIA_IDX, tmp);
        if (strlen(tmp) > 4) {
            cmor_handle_error_variadic(
                "Your initialization_index \"%s\" is invalid. \n! "
                "It cannot contains more than 4 digits. \n! ",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        reti = regexec(&regex, tmp, 0, NULL, 0);
        if (reti) {
            cmor_handle_error_variadic(
                "Your initialization_index \"%s\" is invalid. \n! "
                "It must contain only characters between 0 and 9 \n!",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        sscanf(tmp, "%d", &initialization_index);
        snprintf(tmp, CMOR_MAX_STRING, "i%d", initialization_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }

/* -------------------------------------------------------------------- */
/*      physics id (required)                                           */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PHYSICS_IDX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_PHYSICS_IDX, tmp);
        if (strlen(tmp) > 4) {
            cmor_handle_error_variadic(
                "Your physics_index \"%s\" is invalid. \n! "
                "It cannot contains more than 4 digits. \n! ",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        reti = regexec(&regex, tmp, 0, NULL, 0);
        if (reti) {
            cmor_handle_error_variadic(
                "Your physics_index \"%s\" is invalid. \n! "
                "It must contain only characters between 0 and 9 \n!",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        sscanf(tmp, "%d", &physics_index);
        snprintf(tmp, CMOR_MAX_STRING, "p%d", physics_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }
/* -------------------------------------------------------------------- */
/*      forcing id (required)                                           */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_FORCING_IDX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_FORCING_IDX, tmp);
        if (strlen(tmp) > 4) {
            cmor_handle_error_variadic(
                "Your forcing_index \"%s\" is invalid. \n! "
                "It cannot contains more than 4 digits. \n! ",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        reti = regexec(&regex, tmp, 0, NULL, 0);
        if (reti) {
            cmor_handle_error_variadic(
                "Your forcing_index \"%s\" is invalid. \n! "
                "It must contain only characters between 0 and 9 \n!",
                CMOR_NORMAL, tmp);
            ierr += -1;

        }
        sscanf(tmp, "%d", &forcing_index);

        snprintf(tmp, CMOR_MAX_STRING, "f%d", forcing_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VARIANT_LABEL, variant,
                                            1);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_MEMBER_ID, variant, 1);

    // append sub_experiment_id
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID, szValue);
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_MEMBER_ID, szVariant);
        if (strcmp(szValue, NONE) != 0) {
            // not already in variant
            if (strstr(szVariant, szValue) == NULL) {
                strcat(szValue, "-");
                strcat(szValue, szVariant);
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_MEMBER_ID,
                                                        szValue, 1);
            }
        }
    }

    regfree(&regex);
    cmor_pop_traceback();
    return (ierr);

}
/************************************************************************/
/*                        cmor_build_outname()                          */
/************************************************************************/
int cmor_build_outname(int var_id, char *outname ) {
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];
    cdCalenType icalo;
    cdCompTime starttime, endtime;
    int i,j;
    int n;

    /* -------------------------------------------------------------------- */
    /*      ok at that point we need to construct the final name!           */
    /* -------------------------------------------------------------------- */
    if (cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_table_id].
            axes[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_axis_id].axis
            == 'T') {
        cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[0], "units", 'c',
                &msg);
        cmor_get_cur_dataset_attribute("calendar", msg2);

        if (cmor_calendar_c2i(msg2, &icalo) != 0) {
            cmor_handle_error_var_variadic(
                "Cannot convert times for calendar: %s,\n! "
                "closing variable %s (table: %s)",
                CMOR_CRITICAL, var_id,
                msg2, cmor_vars[var_id].id,
                cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }
        /* -------------------------------------------------------------------- */
        /*      ok makes a comptime for start and end time                      */
        /* -------------------------------------------------------------------- */

        i = cmor_vars[var_id].axes_ids[0];
        j = cmor_axes[i].ref_table_id;
        i = cmor_axes[i].ref_axis_id;
        if (cmor_tables[j].axes[i].climatology == 1) {
            starttime.year = 0;
            starttime.month = 0;
            starttime.day = 0;
            starttime.hour = 0.0;
            endtime = starttime;
            cdRel2Comp(icalo, msg, cmor_vars[var_id].first_bound, &starttime);
            cdRel2Comp(icalo, msg, cmor_vars[var_id].last_bound, &endtime);
        } else {
            cdRel2Comp(icalo, msg, cmor_vars[var_id].first_time, &starttime);
            cdRel2Comp(icalo, msg, cmor_vars[var_id].last_time, &endtime);
        }

        /* -------------------------------------------------------------------- */
        /*      We want start and end times that are greater than               */
        /*      x minutes 59.5 seconds to be set to x+1 minutes 0 seconds       */
        /*      so add a half second so that when floats are converted to       */
        /*       integers, this will round to nearest second (we know that      */
        /*       the time coordinates are positive                              */
        /*                                                                      */
        /*    note that in the following, cdCompAdd expects the increment added */
        /*      to be expressed in units of hours                               */
        /* -------------------------------------------------------------------- */

        if (icalo == cdMixed) {
            cdCompAddMixed(starttime, 0.5 / 3600., &starttime);
            cdCompAddMixed(endtime, 0.5 / 3600., &endtime);

        } else {
            cdCompAdd(starttime, 0.5 / 3600., icalo, &starttime);
            cdCompAdd(endtime, 0.5 / 3600., icalo, &endtime);
        }
        /* -------------------------------------------------------------------- */
        /*      need to figure out the frequency                                */
        /* -------------------------------------------------------------------- */
        int frequency_code;
        char frequency[CMOR_MAX_STRING];
        char start_string[CMOR_MAX_STRING];
        char end_string[CMOR_MAX_STRING];
        int start_seconds, end_seconds, start_minutes, end_minutes;

        strncpy(frequency, cmor_vars[var_id].frequency, CMOR_MAX_STRING);

        if (strstr(frequency, "yr") != NULL) {
            frequency_code = 1;
        } else if (strstr(frequency, "dec") != NULL) {
            frequency_code = 1;
        } else if (strstr(frequency, "monC") != NULL) {
            frequency_code = 6;
        } else if (strstr(frequency, "mon") != NULL) {
            frequency_code = 2;
        } else if (strstr(frequency, "day") != NULL) {
            frequency_code = 3;
        } else if (strstr(frequency, "subhr") != NULL) {
            frequency_code = 5;
        } else if (strstr(frequency, "hr") != NULL) {
            frequency_code = 4;
        } else if (strstr(frequency, "fx") != NULL) {
            frequency_code = 99;
        } else {
            frequency_code = 0;
        }

        switch (frequency_code) {
        case 1:
            /* frequency is yr, decadal */
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld", starttime.year);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld", endtime.year);
            break;
        case 2:
            /* frequency is mon */
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i", starttime.year,
                    starttime.month);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i", endtime.year,
                    endtime.month);
            break;
        case 3:
            /* frequency is day */
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i",
                    starttime.year, starttime.month, starttime.day);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i", endtime.year,
                    endtime.month, endtime.day);
            break;
        case 4:
            /* frequency is 6hr, 3hr, 1hr */
            /* round to the nearest minute */
            start_minutes = round(
                    (starttime.hour - (int) starttime.hour) * 60.);
            end_minutes = round((endtime.hour - (int) endtime.hour) * 60.);
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i",
                    starttime.year, starttime.month, starttime.day,
                    (int) starttime.hour, start_minutes);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i",
                    endtime.year, endtime.month, endtime.day,
                    (int) endtime.hour, end_minutes);
            break;
        case 5:
            /* frequency is subhr */
            /* round to the nearest second */
            start_seconds = (int) ((starttime.hour - (int) starttime.hour)
                    * 3600);
            end_seconds = (int) ((endtime.hour - (int) endtime.hour) * 3600);
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i%.2i",
                    starttime.year, starttime.month, starttime.day,
                    (int) starttime.hour, (int) (start_seconds / 60),
                    (start_seconds % 60));
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i%.2i",
                    endtime.year, endtime.month, endtime.day,
                    (int) endtime.hour, (int) (end_seconds / 60),
                    (end_seconds % 60));

            break;
        case 6:
            // frequency is monC
            // add/subtract 1 hour (this is overkill, but safe)
            // to prevent truncation errors from possibly leading you to
            // the wrong month
            //
            // note that cdCompAdd expects the increment added to be
            //      expressed in units of hours

            if (icalo == cdMixed) {
                cdCompAddMixed(starttime, 1.0, &starttime);
                cdCompAddMixed(endtime, -1.0, &endtime);
            } else {
                cdCompAdd(starttime, 1.0, icalo, &starttime);
                cdCompAdd(endtime, -1.0, icalo, &endtime);
            }
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i", starttime.year,
                    starttime.month);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i", endtime.year,
                    endtime.month);
            break;
        case 99:
            /* frequency is fx */
            /* don't need to do anything, time string will ignored in next step */
            break;
        default:
            cmor_handle_error_var_variadic(
                "Cannot find frequency %s. Closing variable %s (table: %s)",
                CMOR_CRITICAL, var_id,
                frequency, cmor_vars[var_id].id,
                cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }

        strncat(outname, "_", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, start_string, CMOR_MAX_STRING - strlen(outname));
        strncat(outname, "-", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, end_string, CMOR_MAX_STRING - strlen(outname));

        if (cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_table_id].axes[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_axis_id].climatology
                == 1) {
            strncat(outname, "-clim", CMOR_MAX_STRING - strlen(outname));
        }
    }

    if (cmor_vars[var_id].suffix_has_date == 1) {
        /* -------------------------------------------------------------------- */
        /*      all right we need to pop out the date part....                  */
        /* -------------------------------------------------------------------- */

        n = strlen(cmor_vars[var_id].suffix);
        i = 0;
        while (cmor_vars[var_id].suffix[i] != '_')
            i++;
        i++;
        while ((cmor_vars[var_id].suffix[i] != '_') && i < n)
            i++;
        /* -------------------------------------------------------------------- */
        /*      ok now we have the length of dates                              */
        /*      at this point we are either at the                              */
        /*      _clim the actual _suffix or the end (==nosuffix)                */
        /*      checking if _clim needs to be added                             */
        /* -------------------------------------------------------------------- */
        if (cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_table_id].axes[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_axis_id].climatology
                == 1) {
            i += 5;
        }
        strcpy(msg, "");
        for (j = i; j < n; j++) {
            msg[j - i] = cmor_vars[var_id].suffix[i];
            msg[j - i + 1] = '\0';
        }
    } else {
        strncpy(msg, cmor_vars[var_id].suffix, CMOR_MAX_STRING);
    }

    if (strlen(msg) > 0) {
        strncat(outname, "_", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, msg, CMOR_MAX_STRING - strlen(outname));
    }
    strncat(outname, ".nc", CMOR_MAX_STRING - strlen(outname));
    return(0);
}
/************************************************************************/
/*                        cmor_close_variable()                         */
/************************************************************************/
int cmor_close_variable(int var_id, char *file_name, int *preserve)
{
    int ierr;
    extern int cmor_nvars;
    char outname[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    // char msg2[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    // cdCalenType icalo;
    // cdCompTime starttime, endtime;
    int i, j;

    cmor_add_traceback("cmor_close_variable");
    cmor_is_setup();

    cleanup_varid = var_id;
/* -------------------------------------------------------------------- */
/*  initialized contains ncic, so we close file only once.              */
/* -------------------------------------------------------------------- */
    if (cmor_vars[var_id].initialized != -1 && cmor_vars[var_id].error == 0) {
        ierr = nc_close(cmor_vars[var_id].initialized);

        if (ierr != NC_NOERR) {
            cmor_handle_error_var_variadic(
                "NetCDF Error (%i: %s) closing variable %s (table: %s)\n! ",
                CMOR_CRITICAL, var_id,
                ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }

/* -------------------------------------------------------------------- */
/*      Ok we need to make the associated variables                     */
/*      have been written in the case of a time varying grid            */
/* -------------------------------------------------------------------- */
        if ((cmor_vars[var_id].grid_id > -1)
            && (cmor_grids[cmor_vars[var_id].grid_id].istimevarying == 1)) {
            for (i = 0; i < 4; i++) {
                if (cmor_grids[cmor_vars[var_id].grid_id].associated_variables
                    [i] != -1) {
/* -------------------------------------------------------------------- */
/*      ok this associated coord should be stored                       */
/* -------------------------------------------------------------------- */
                    if (cmor_vars[var_id].ntimes_written !=
                        cmor_vars[var_id].ntimes_written_coords[i]) {
/* -------------------------------------------------------------------- */
/*      ok we either wrote more or less data but                        */
/*      in any case not the right amount!                               */
/* -------------------------------------------------------------------- */

                        if (cmor_vars[var_id].ntimes_written == 0) {
                            for (j = 0; j < cmor_vars[var_id].ndims; j++) {
                                if (cmor_axes
                                    [cmor_vars[var_id].axes_ids[j]].axis ==
                                    'T') {
                                    sprintf(ctmp2, "%i",
                                            cmor_axes[cmor_vars
                                                      [var_id].axes_ids
                                                      [j]].length);
                                    break;
                                }
                            }
                        } else {
                            sprintf(ctmp2, "%i",
                                    cmor_vars[var_id].ntimes_written);
                        }
                        if (cmor_vars[var_id].ntimes_written_coords[i] == -1) {
                            sprintf(ctmp, "no");
                        } else {
                            sprintf(ctmp, "%i",
                                    cmor_vars[var_id].ntimes_written_coords[i]);
                        }
                        cmor_handle_error_var_variadic(
                            "while closing variable %i (%s, table %s)\n! "
                            "we noticed it has a time varying grid, \n! "
                            "you wrote %s time steps for the variable,\n! "
                            "but its associated variable %i (%s) has\n! "
                            "%s times written",
                            CMOR_CRITICAL, var_id,
                            cmor_vars[var_id].self,
                            cmor_vars[var_id].id,
                            cmor_tables[cmor_vars[var_id].
                                        ref_table_id].szTable_id, ctmp2,
                            cmor_vars[cmor_grids
                                    [cmor_vars[var_id].grid_id].
                                    associated_variables[i]].self,
                            cmor_vars[cmor_grids
                                    [cmor_vars[var_id].grid_id].
                                    associated_variables[i]].id, ctmp);
                        cmor_pop_traceback();
                        return (1);

                    }
                }
            }
        }
        for (i = 0; i < 10; i++) {
            if (cmor_vars[var_id].associated_ids[i] != -1) {
                if (cmor_vars[var_id].ntimes_written !=
                    cmor_vars[var_id].ntimes_written_associated[i]) {
                    sprintf(ctmp2, "%i", cmor_vars[var_id].ntimes_written);
                    sprintf(ctmp, "%i",
                            cmor_vars[var_id].ntimes_written_associated[i]);
                    cmor_handle_error_var_variadic(
                        "while closing variable %i (%s, table %s) we\n! "
                        "noticed it has a time varying associated\n! "
                        "variable, you wrote %s time steps for the\n! "
                        "variable, but its associated variable %i (%s)\n! "
                        "has %s times written",
                        CMOR_CRITICAL, var_id,
                        cmor_vars[var_id].self, cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id, ctmp2,
                        cmor_vars[cmor_vars[var_id].associated_ids[i]].
                        self,
                        cmor_vars[cmor_vars[var_id].associated_ids[i]].id,
                        ctmp);
                    cmor_pop_traceback();
                    return (1);

                }
            }
        }

/* -------------------------------------------------------------------- */
/*    Check if the number of times written is less than the             */
/*    length of the time axis.                                          */
/* -------------------------------------------------------------------- */
        for(i = 0; i < cmor_vars[var_id].ndims; ++i) {
            if(cmor_axes[cmor_vars[var_id].axes_ids[i]].axis == 'T'
                && cmor_axes[cmor_vars[var_id].axes_ids[i]].length > 0
                && (cmor_vars[var_id].ntimes_written 
                < cmor_axes[cmor_vars[var_id].axes_ids[i]].length)) {
                cmor_handle_error_var_variadic(
                    "while closing variable %i (%s, table %s)\n! "
                    "we noticed you wrote %i time steps for the variable,\n! "
                    "but its time axis %i (%s) has %i time steps",
                    CMOR_WARNING, var_id,
                    cmor_vars[var_id].self,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id,
                    cmor_vars[var_id].ntimes_written, i,
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].length);
            }
        }

        strncpytrim( outname, cmor_vars[var_id].base_path,
                 CMOR_MAX_STRING );

        ierr = cmor_build_outname(var_id, outname);
        if(ierr != 0) {
            return(1);
        }
/* -------------------------------------------------------------------- */
/*      ok now we can actually move the file                            */
/*      here we need to make sure we are not in preserve mode!          */
/* -------------------------------------------------------------------- */
        if ((CMOR_NETCDF_MODE == CMOR_PRESERVE_4)
            || (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)) {
            FILE *fperr;

/* -------------------------------------------------------------------- */
/*      ok first let's check if the file does exists or not             */
/* -------------------------------------------------------------------- */
            fperr = NULL;
            fperr = fopen(outname, "r");
            if (fperr != NULL) {
                sprintf(msg, "%s.copy", outname);
                if (rename(outname, msg) == 0) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Output file ( %s ) already exists,\n! "
                             "remove file or use CMOR_REPLACE or\n! "
                             "CMOR_APPEND for CMOR_NETCDF_MODE value\n! "
                             "in cmor_setup for convenience the file\n! "
                             "you were trying to write has been saved\n! "
                             "at: %s.copy", outname, outname);
                } else {
                    cmor_handle_error_var_variadic(
                        "Output file ( %s ) already exists,\n! "
                        "remove file or use CMOR_REPLACE or\n! "
                        "CMOR_APPEND for CMOR_NETCDF_MODE value in\n! "
                        "cmor_setup.",
                        CMOR_CRITICAL, var_id, outname);
                }
                ierr = fclose(fperr);
                fperr = NULL;
            }
        }
        ierr = rename(cmor_vars[var_id].current_path, outname);
        if (ierr != 0) {
            cmor_handle_error_var_variadic(
                "could not rename temporary file: %s to final file\n"
                "name: %s",
                CMOR_CRITICAL, var_id,
                cmor_vars[var_id].current_path, outname);
        }
        if (file_name != NULL) {
            strncpy(file_name, outname, CMOR_MAX_STRING);
        }
        strncpy(cmor_current_dataset.finalfilename, outname, CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*      At this point we need to check the file's size                  */
/*      and issue a warning if greater than 4Gb                         */
/* -------------------------------------------------------------------- */
//        stat(outname, &buf);
//        sz = buf.st_size;
//        if (sz > maxsz) {
//            sprintf(msg, "Closing file: %s size is greater than 4Gb, while\n! "
//                    "this is acceptable \n! ", outname);
//            cmor_handle_error_var(msg, CMOR_WARNING, var_id);
//        }

        if (preserve != NULL) {
            cmor_vars[var_id].initialized = -1;
            cmor_vars[var_id].ntimes_written = 0;
            cmor_vars[var_id].time_nc_id = -999;
            cmor_vars[var_id].time_bnds_nc_id = -999;
            for (i = 0; i < 10; i++) {
                cmor_vars[var_id].ntimes_written_coords[i] = -1;
                cmor_vars[var_id].ntimes_written_associated[i] = 0;
                cmor_vars[var_id].associated_ids[i] = -1;
                cmor_vars[var_id].nc_var_id = -999;
            }
            if (cmor_vars[var_id].values != NULL) {
                free(cmor_vars[var_id].values);
            }
            for (i = 0; i < cmor_vars[var_id].nattributes; i++) {
                if (strcmp(cmor_vars[var_id].attributes[i], "cell_methods")
                    == 0) {
                    cmor_set_variable_attribute_internal(var_id, "cell_methods",
                                                         'c',
                                                         cmor_tables[cmor_vars
                                                                     [var_id].
                                                                     ref_table_id].vars
                                                         [cmor_vars[var_id].
                                                          ref_var_id].
                                                         cell_methods);
                }
            }
        } else {
            cmor_reset_variable(var_id);
            cmor_vars[var_id].closed = 1;
        }
    }
    cleanup_varid = -1;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                             cmor_close()                             */
/************************************************************************/
int cmor_close(void)
{
    int i, j, k;
    extern int cmor_nvars;
    extern ut_system *ut_read;
    extern FILE *output_logfile;

    cmor_add_traceback("cmor_close");
    cmor_is_setup();
    if (output_logfile == NULL)
        output_logfile = stderr;

    for (i = 0; i < cmor_nvars + 1; i++) {
        if (cmor_vars[i].initialized != -1 && cmor_vars[i].error == 0) {
            if (cmor_vars[i].closed == 0) {
                cmor_close_variable(i, NULL, NULL);
            }
        } else if ((cmor_vars[i].needsinit == 1)
                   && (cmor_vars[i].closed != 1)) {
            cmor_handle_error_variadic(
                "variable %s (%i, table: %s) has been defined\n! "
                "but never initialized",
                CMOR_WARNING,
                cmor_vars[i].id, i,
                cmor_tables[cmor_vars[i].ref_table_id].szTable_id);
        } else if (cmor_vars[i].zaxis != -1) {
            cmor_reset_variable(i);
        }
    }
    for (i = 0; i < CMOR_MAX_TABLES; i++) {
        for (j = 0; j < CMOR_MAX_ELEMENTS; j++) {
            if (cmor_tables[i].axes[j].requested != NULL) {
                free(cmor_tables[i].axes[j].requested);
                cmor_tables[i].axes[j].requested = NULL;
            }
            if (cmor_tables[i].axes[j].requested_bounds != NULL) {
                free(cmor_tables[i].axes[j].requested_bounds);
                cmor_tables[i].axes[j].requested_bounds = NULL;
            }
            if (cmor_tables[i].axes[j].crequested != NULL) {
                free(cmor_tables[i].axes[j].crequested);
                cmor_tables[i].axes[j].crequested = NULL;
            }
        }
        if (cmor_tables[i].nforcings > 0) {
            for (j = 0; j < cmor_tables[i].nforcings; j++) {
                free(cmor_tables[i].forcings[j]);
                cmor_tables[i].forcings[j] = NULL;
            }
            free(cmor_tables[i].forcings);
            cmor_tables[i].forcings = NULL;
            cmor_tables[i].nforcings = 0;
        }
        if (cmor_tables[i].CV != NULL) {
            for (k = 0; k < cmor_tables[i].CV->nbObjects; k++) {
                if (&cmor_tables[i].CV[k] != NULL) {
                    cmor_CV_free(&cmor_tables[i].CV[k]);
                }
            }
            if (cmor_tables[i].CV != NULL) {
                free(cmor_tables[i].CV);
                cmor_tables[i].CV = NULL;
            }
        }

    }

    for (i = 0; i < CMOR_MAX_GRIDS; i++) {
        if (cmor_grids[i].lons != NULL) {
            free(cmor_grids[i].lons);
            cmor_grids[i].lons = NULL;
        }
        if (cmor_grids[i].lats != NULL) {
            free(cmor_grids[i].lats);
            cmor_grids[i].lats = NULL;
        }
        if (cmor_grids[i].blons != NULL) {
            free(cmor_grids[i].blons);
            cmor_grids[i].blons = NULL;
        }
        if (cmor_grids[i].blats != NULL) {
            free(cmor_grids[i].blats);
            cmor_grids[i].blats = NULL;
        }
    }
    if ((cmor_nerrors != 0 || cmor_nwarnings != 0)) {
        fprintf(output_logfile, "! ------\n! CMOR is now closed.\n! ------\n! "
                "During execution we encountered:\n! ");
#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%d;%dm", 0X1B, 1, 34);
#endif
        fprintf(output_logfile, "%3i Warning(s)", cmor_nwarnings);
#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%dm", 0X1B, 0);
#endif
        fprintf(output_logfile, "\n! ");
#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%d;%dm", 0X1B, 1, 31);
#endif
        fprintf(output_logfile, "%3i Error(s)", cmor_nerrors);
#ifdef COLOREDOUTPUT
        fprintf(output_logfile, "%c[%dm", 0X1B, 0);
#endif
        fprintf(output_logfile,
                "\n! ------\n! Please review them.\n! ------\n! \n");
        cmor_nerrors = 0;
        cmor_nwarnings = 0;
    } else {
        fprintf(output_logfile,
                "\n! ------\n! All files were closed successfully. \n! ------\n! \n");
    }
    if (output_logfile != stderr) {
        fclose(output_logfile);
        output_logfile = NULL;
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_getFinalFilenane()                        */
/************************************************************************/
char *cmor_getFinalFilename()
{
    cmor_add_traceback("cmor_close");
    cmor_is_setup();

    if (cmor_current_dataset.finalfilename[0] != '\0') {
        cmor_pop_traceback();
        return (cmor_current_dataset.finalfilename);
    }
    cmor_pop_traceback();
    return (NULL);

}

/************************************************************************/
/*                          cmor_trim_string()                          */
/************************************************************************/
void cmor_trim_string(char *in, char *out)
{
    int n, i, j;

    if (in == NULL) {
        out = NULL;
        return;
    }
    n = strlen(in);

    if (n == 0) {
        out[0] = '\0';
        return;
    }
    if (n > CMOR_MAX_STRING)
        n = CMOR_MAX_STRING;
    j = 0;
    for (i = 0; i < n; i++) {
        if (in[i] != ' ' && in[i] != '\n' && in[i] != '\t') {
            break;
        } else {
            j++;
        }
    }
    for (i = j; i < n; i++) {
        out[i - j] = in[i];
    }
    out[i - j] = '\0';
    n = strlen(out);
    i = n;
    while ((out[i] == '\0' || out[i] == ' ')) {
        out[i] = '\0';
        i--;
    }
}
