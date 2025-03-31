#include <time.h>
#include <stdio.h>
#include <string.h>
#include "cmor.h"
#include <netcdf.h>
#include <udunits2.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>

float fvalue;

extern volatile sig_atomic_t stop;

/************************************************************************/
/*                cmor_is_required_variable_attribute()                 */
/************************************************************************/
int cmor_is_required_variable_attribute(cmor_var_def_t var,
                                        char *attribute_name)
{

    char astr[CMOR_MAX_STRING];
    int i, j;

    if (var.required[0] == '\0') {
        return (1);
    }

    i = 0;
    astr[0] = '\0';
    j = 0;

    while (var.required[i] != '\0') {

        while ((var.required[i] != ' ') && (var.required[i] != '\0')) {
            astr[j] = var.required[i];
            i += 1;
            j += 1;
        }

        astr[j] = '\0';

        if (strncmp(astr, attribute_name, CMOR_MAX_STRING) == 0) {
            return (0);
        }

        j = 0;
        astr[0] = '\0';
        while (var.required[i] == ' ') {
            i += 1;
        }
    }
    return (1);
}

/************************************************************************/
/*               cmor_has_required_variable_attributes()                */
/************************************************************************/
int cmor_has_required_variable_attributes(int var_id)
{
    extern cmor_var_t cmor_vars[];
    char astr[CMOR_MAX_STRING];
    int i, j;
    cmor_var_def_t var;
    cmor_table_t *pTable;

    cmor_add_traceback("cmor_has_required_variable_attributes");

    pTable = &cmor_tables[cmor_vars[var_id].ref_table_id];
    var = pTable->vars[cmor_vars[var_id].ref_var_id];

    if (var.required[0] == '\0') {
        cmor_pop_traceback();
        return (0);
    }

    i = 0;
    astr[0] = '\0';
    j = 0;

    while (var.required[i] != '\0') {
        while ((var.required[i] != ' ') && (var.required[i] != '\0')) {
            astr[j] = var.required[i];
            i += 1;
            j += 1;
        }

        astr[j] = '\0';

        if (cmor_has_variable_attribute(var_id, astr) != 0) {

            cmor_handle_error_var_variadic(
                "variable %s (table %s) does not have required "
                "attribute: %s",
                CMOR_NORMAL, var_id,
                cmor_vars[var_id].id, pTable->szTable_id, astr);
            cmor_pop_traceback();
            return (-1);
        }

        j = 0;
        astr[0] = '\0';

        while (var.required[i] == ' ')
            i += 1;
    }
    cmor_pop_traceback();
    return (0);

}

/************************************************************************/
/*                cmor_set_variable_attribute_internal()                */
/************************************************************************/
int cmor_set_variable_attribute_internal(int id, char *attribute_name,
                                         char type, void *value)
{
    extern cmor_var_t cmor_vars[];
    int i, index;
    char msg[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_set_variable_attribute_internal");

    cmor_is_setup();
    index = -1;
    cmor_trim_string(attribute_name, msg);

    for (i = 0; i < cmor_vars[id].nattributes; i++) {
        if (strcmp(cmor_vars[id].attributes[i], msg) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        index = cmor_vars[id].nattributes;
        cmor_vars[id].nattributes += 1;
    }

    /*stores the name */

    strncpy(cmor_vars[id].attributes[index], msg, CMOR_MAX_STRING);

    cmor_vars[id].attributes_type[index] = type;
    cmor_vars[id].attributes_values_num[index] = (double)*(float *)value;
    if (type == 'c') {

        if (strlen(value) > 0) {
            strncpytrim(cmor_vars[id].attributes_values_char[index], value,
                        CMOR_MAX_STRING);
        } else {
            strcpy(cmor_vars[id].attributes[index], "");
        }

    } else if (type == 'f') {

        cmor_vars[id].attributes_values_num[index] = (double)*(float *)value;
    } else if (type == 'i') {

        cmor_vars[id].attributes_values_num[index] = (double)*(int *)value;
    } else if (type == 'd') {

        cmor_vars[id].attributes_values_num[index] = (double)*(double *)value;
    } else if (type == 'l') {

        cmor_vars[id].attributes_values_num[index] = (double)*(long *)value;
    } else {
        cmor_handle_error_var_variadic(
            "unknown type %c for attribute %s of variable %s "
            "(table %s),allowed types are c,i,l,f,d",
            CMOR_NORMAL, id,
            type,
            attribute_name, cmor_vars[id].id,
            cmor_tables[cmor_vars[id].ref_table_id].szTable_id);
        cmor_pop_traceback();
        return (1);
    }

    if ((type != 'c') && (type != cmor_vars[id].type)) {
        cmor_handle_error_var_variadic(
            "Type '%c' for attribute '%s' of variable '%s' "
            "does not match type variable '%c'",
            CMOR_WARNING, id,
            type, attribute_name,
            cmor_vars[id].id, cmor_vars[id].type);
    }

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_set_variable_attribute()                     */
/************************************************************************/
int cmor_set_variable_attribute(int id, char *attribute_name, char type,
                                void *value)
{
    cmor_add_traceback("cmor_set_variable_attribute");

/* -------------------------------------------------------------------- */
/*       First of all we need to see if it is not one of the args       */
/*       you can set by calling cmor_variable                           */
/* -------------------------------------------------------------------- */

    if ((strcmp(attribute_name, VARIABLE_ATT_UNITS) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_MISSINGVALUES) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_FILLVAL) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_STANDARDNAME) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_LONGNAME) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_FLAGVALUES) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_FLAGMEANING) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_COMMENT) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_ORIGINALNAME) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_ORIGINALUNITS) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_POSITIVE) == 0) ||
        (strcmp(attribute_name, VARIABLE_ATT_CELLMETHODS) == 0)) {
        cmor_handle_error_var_variadic(
            "variable attribute %s (for variable %s, table %s) must be "
            "set via a call to cmor_variable or it is automatically set "
            "via the tables",
            CMOR_NORMAL, id,
            attribute_name, cmor_vars[id].id,
            cmor_tables[cmor_vars[id].ref_table_id].szTable_id);
        cmor_pop_traceback();
        return (1);
    }
/* -------------------------------------------------------------------- */
/*      Before setting the attribute we need to see if the variable     */
/*      has been initialized                                            */
/* -------------------------------------------------------------------- */
    if (cmor_vars[id].initialized != -1) {
        cmor_handle_error_var_variadic(
            "attribute %s on variable %s (table %s) will probably not be "
            "set as the variable has already been created into the output "
            "NetCDF file, please place this call BEFORE any call to "
            "cmor_write",
            CMOR_NORMAL, id,
            attribute_name, cmor_vars[id].id,
            cmor_tables[cmor_vars[id].ref_table_id].szTable_id);
        cmor_pop_traceback();
        return (1);
    }
    cmor_pop_traceback();
    return (cmor_set_variable_attribute_internal(id, attribute_name, type,
                                                 value));
}

/************************************************************************/
/*                    cmor_get_variable_attribute()                     */
/************************************************************************/
int cmor_get_variable_attribute(int id, char *attribute_name, void *value)
{
    extern cmor_var_t cmor_vars[];
    int i, index;
    char type;

    cmor_add_traceback("cmor_get_variable_attribute");
    cmor_is_setup();
    index = -1;
    for (i = 0; i < cmor_vars[id].nattributes; i++) {
        if (strcmp(cmor_vars[id].attributes[i], attribute_name) == 0) {
            index = i;
            break;
        }                       /* we found it */
    }
    if (index == -1) {
        cmor_handle_error_var_variadic(
            "Attribute %s could not be found for variable %i (%s, table: %s)",
            CMOR_NORMAL, id,
            attribute_name, id, cmor_vars[id].id,
            cmor_tables[cmor_vars[id].ref_table_id].szTable_id);
        cmor_pop_traceback();
        return (1);
    }
    type = cmor_vars[id].attributes_type[i];
    if (type == 'c')
        strncpy(value, cmor_vars[id].attributes_values_char[index],
                CMOR_MAX_STRING);
    else if (type == 'f')
        *(float *)value = (float)cmor_vars[id].attributes_values_num[index];
    else if (type == 'i')
        *(int *)value = (int)cmor_vars[id].attributes_values_num[index];
    else if (type == 'l')
        *(long *)value = (long)cmor_vars[id].attributes_values_num[index];
    else
        *(double *)value = (double)cmor_vars[id].attributes_values_num[index];
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_has_variable_attribute()                     */
/************************************************************************/
int cmor_has_variable_attribute(int id, char *attribute_name)
{
    extern cmor_var_t cmor_vars[];
    int i, index;
    char type;
    char msg[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_has_variable_attribute");
    cmor_is_setup();
    index = -1;
    for (i = 0; i < cmor_vars[id].nattributes; i++) {
        if (strcmp(cmor_vars[id].attributes[i], attribute_name) == 0) {
            index = i;
            break;
        }
    }
    if ((index == -1) || strlen(attribute_name) == 0) {
        cmor_pop_traceback();
        return (1);
    }
    i = 0;
    /* if it is empty we assume not defined */
    cmor_get_variable_attribute_type(id, attribute_name, &type);
    if (type == 'c') {
        cmor_get_variable_attribute(id, attribute_name, msg);
        if (strlen(msg) == 0) {
            /* empty string attribute has been deleted */
            i = 1;
        }
    }
    cmor_pop_traceback();
    return (i);
}

/************************************************************************/
/*                 cmor_get_variable_attribute_names()                  */
/************************************************************************/
int cmor_get_variable_attribute_names(int id, int *nattributes,
                                      char attributes_names[]
                                      [CMOR_MAX_STRING])
{
    extern cmor_var_t cmor_vars[];
    int i;

    cmor_add_traceback("cmor_get_variable_attribute_names");
    cmor_is_setup();
    *nattributes = cmor_vars[id].nattributes;
    for (i = 0; i < cmor_vars[id].nattributes; i++) {
        strncpy(attributes_names[i], cmor_vars[id].attributes[i],
                CMOR_MAX_STRING);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                 cmor_get_variable_attribute_type()                   */
/************************************************************************/
int cmor_get_variable_attribute_type(int id, char *attribute_name, char *type)
{

    extern cmor_var_t cmor_vars[];
    int i, index;

    cmor_add_traceback("cmor_get_variable_attribute_type");
    cmor_is_setup();
    index = -1;

    for (i = 0; i < cmor_vars[id].nattributes; i++) {
        if (strcmp(cmor_vars[id].attributes[i], attribute_name) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        cmor_handle_error_var_variadic(
            "Attribute %s could not be found for variable %i (%s, table: %s)",
            CMOR_NORMAL, id,
            attribute_name, id, cmor_vars[id].id,
            cmor_tables[cmor_vars[id].ref_table_id].szTable_id);
        cmor_pop_traceback();
        return (1);
    }

    *type = cmor_vars[id].attributes_type[i];
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                            cmor_zfactor()                            */
/************************************************************************/
int cmor_zfactor(int *zvar_id, int axis_id, char *name, char *units,
                 int ndims, int axes_ids[], char type, void *values,
                 void *bounds)
{

    extern int cmor_nvars;

    int i, j, k;
    int n, gid;
    int var_id;

    char msg[CMOR_MAX_STRING];
    extern ut_system *ut_read;
    ut_unit *user_units, *cmor_units;
    cv_converter *ut_cmor_converter;
    char local_unit[CMOR_MAX_STRING];
    double tmp;
    char comment[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_zfactor");
    cmor_is_setup();

    strcpy(comment, COMMENT_VARIABLE_ZFACTOR);
/* -------------------------------------------------------------------- */
/*      first check if we need to convert values                        */
/* -------------------------------------------------------------------- */
    if (cmor_axes[axis_id].hybrid_out == cmor_axes[axis_id].hybrid_in) {

/* -------------------------------------------------------------------- */
/*      no it's a normal hybrid, no conv                                */
/* -------------------------------------------------------------------- */
        i = cmor_variable(&var_id, name, units, ndims, axes_ids, type,
                          NULL, NULL, NULL, NULL, NULL, comment);
        cmor_vars[var_id].needsinit = 0;
        cmor_vars[var_id].zaxis = axis_id;
        if (values != NULL) {
            n = 1;
            for (i = 0; i < ndims; i++) {
                if (axes_ids[i] > -1) {
                    n *= cmor_axes[axes_ids[i]].length;
                } else {
/* -------------------------------------------------------------------- */
/*      ok irregular grid                                               */
/* -------------------------------------------------------------------- */
                    gid = -axes_ids[i] - CMOR_MAX_GRIDS;
                    for (j = 0; j < cmor_grids[gid].ndims; j++) {
                        n *= cmor_axes[cmor_grids[gid].axes_ids[j]].length;
                    }
                }
            }
            cmor_vars[var_id].values = malloc(n * sizeof(double));
            if (cmor_vars[var_id].values == NULL) {
                cmor_handle_error_var_variadic(
                    "cmor_zfactor: zaxis %s, cannot allocate "
                    "memory for %i double elts %s var '%s' (table: %s)",
                    CMOR_CRITICAL, var_id,
                    cmor_axes[axis_id].id, n, cmor_vars[var_id].id,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            for (i = 0; i < n; i++) {
                if (type == 'd') {
                    cmor_vars[var_id].values[i] = (double)((double *)values)[i];
                } else if (type == 'f') {
                    cmor_vars[var_id].values[i] = (double)((float *)values)[i];
                } else if (type == 'l') {
                    cmor_vars[var_id].values[i] = (double)((long *)values)[i];
                } else if (type == 'i') {
                    cmor_vars[var_id].values[i] = (double)((int *)values)[i];
                }
            }

/* -------------------------------------------------------------------- */
/*      ok we may need to convert to some decent untis                  */
/* -------------------------------------------------------------------- */
            strncpy(local_unit, cmor_vars[var_id].ounits, CMOR_MAX_STRING);
            cmor_units = ut_parse(ut_read, local_unit, UT_ASCII);

            if (ut_get_status() != UT_SUCCESS) {
                cmor_handle_error_var_variadic(
                    "Udunits: Error parsing units: %s, zaxis: "
                    "%s, variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    local_unit, cmor_axes[axis_id].id,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            strncpy(local_unit, units, CMOR_MAX_STRING);
            ut_trim(local_unit, UT_ASCII);
            user_units = ut_parse(ut_read, local_unit, UT_ASCII);
            if (ut_get_status() != UT_SUCCESS) {

                cmor_handle_error_var_variadic(
                         "Udunits: Error parsing units: %s, zaxis %s, "
                         "variable %s (table: %s)",
                         CMOR_CRITICAL, var_id,
                         local_unit, cmor_axes[axis_id].id,
                         cmor_vars[var_id].id,
                         cmor_tables[cmor_vars[var_id].ref_table_id].
                         szTable_id);

            }

            ut_cmor_converter = ut_get_converter(user_units, cmor_units);
            if (ut_get_status() != UT_SUCCESS) {
                cmor_handle_error_var_variadic(
                    "Udunits: Error getting converter from %s to %s, "
                    "zaxis: %s, variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    units, cmor_vars[var_id].ounits,
                    cmor_axes[axis_id].id, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            cv_convert_doubles(ut_cmor_converter,
                               &cmor_vars[var_id].values[0], n,
                               &cmor_vars[var_id].values[0]);

            if (ut_get_status() != UT_SUCCESS) {
                cmor_handle_error_var_variadic(
                    "Udunits: Error with converter (from %s to %s), zaxis: %s, variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    units, cmor_vars[var_id].ounits,
                    cmor_axes[axis_id].id, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            cv_free(ut_cmor_converter);
            if (ut_get_status() != UT_SUCCESS) {
                cmor_handle_error_var_variadic(
                    "Udunits: Error freeing converter, zaxis %s, variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    cmor_axes[axis_id].id, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            ut_free(cmor_units);
            if (ut_get_status() != UT_SUCCESS) {
                cmor_handle_error_var_variadic(
                    "Udunits: Error freeing units %s, zaxis %s, variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    cmor_vars[var_id].ounits, cmor_axes[axis_id].id,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            ut_free(user_units);
            if (ut_get_status() != UT_SUCCESS) {
                cmor_handle_error_var_variadic(
                    "Udunits: Error freeing units %s, zaxis %s,variable %s (table: %s)",
                    CMOR_CRITICAL, var_id,
                    units, cmor_axes[axis_id].id,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }

            cmor_vars[var_id].itype = 'd';
            *zvar_id = var_id;
        } else {
            /* Ok let's check to make sure it has a time axis! */
            int k = 0;
            for (i = 0; i < ndims; i++) {
                if (axes_ids[i] > -1) {
                    if (cmor_axes[axes_ids[i]].axis == 'T') {
                        k = 1;
                        break;
                    }
                } else {
                    /* ok irregular grid */
                    gid = -axes_ids[i] - CMOR_MAX_GRIDS;
                    for (j = 0; j < cmor_grids[gid].ndims; j++) {
                        if (cmor_axes[cmor_grids[gid].axes_ids[j]].axis == 'T') {
                            k = 1;
                            break;
                        }
                    }
                }
            }

            if (k == 0) {
                cmor_handle_error_var_variadic(
                    "zfactor: axis %s, variable %s (table %s), is "
                    "not time dependent and you did not provide "
                    "any values",
                    CMOR_CRITICAL, var_id,
                    cmor_axes[axis_id].id, name,
                    cmor_tables[cmor_vars[var_id].ref_table_id].
                    szTable_id);
            }
            *zvar_id = var_id;
        }
        if (bounds != NULL) {
            if (ndims != 1) {
                cmor_handle_error_variadic(
                    "zfactor axis %s, variable %s (table: %s): you "
                    "passed bounds values but you also declared %i "
                    "dimensions, we will ignore you bounds",
                    CMOR_WARNING,
                    cmor_axes[axis_id].id, name,
                    cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id,
                    ndims);
            } else {
                strncpy(msg, name, CMOR_MAX_STRING);
                strncat(msg, "_bnds", CMOR_MAX_STRING - strlen(msg));
                i = cmor_variable(&var_id, msg, units, ndims, axes_ids,
                                  'd', NULL, NULL, NULL, NULL, NULL, comment);
                cmor_vars[var_id].zaxis = axis_id;
                cmor_vars[var_id].needsinit = 0;
                n = cmor_axes[axes_ids[0]].length;
                cmor_vars[var_id].values = malloc(2 * n * sizeof(double));
                if (cmor_vars[var_id].values == NULL) {
                    cmor_handle_error_var_variadic(
                        "cmor_zfactor: zaxis %s, cannot allocate "
                        "memory for %i double bounds elts %s var '%s' "
                        "(table: %s)",
                        CMOR_CRITICAL, var_id,
                        cmor_axes[axis_id].id, 2 * n,
                        cmor_vars[var_id].id, cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);
                }

                cmor_vars[var_id].isbounds = 1;
                for (i = 0; i < n; i++) {

                    if (type == 'd') {
                        cmor_vars[var_id].values[2 * i] =
                          (double)((double *)bounds)[i];
                        cmor_vars[var_id].values[2 * i + 1] =
                          (double)((double *)bounds)[i + 1];

                    } else if (type == 'f') {

                        cmor_vars[var_id].values[2 * i] =
                          (double)((float *)bounds)[i];
                        cmor_vars[var_id].values[2 * i + 1] =
                          (double)((float *)bounds)[i + 1];
                    } else if (type == 'l') {

                        cmor_vars[var_id].values[2 * i] =
                          (double)((long *)bounds)[i];
                        cmor_vars[var_id].values[2 * i + 1] =
                          (double)((long *)bounds)[i + 1];

                    } else if (type == 'i') {

                        cmor_vars[var_id].values[2 * i] =
                          (double)((int *)bounds)[i];
                        cmor_vars[var_id].values[2 * i + 1] =
                          (double)((int *)bounds)[i + 1];

                    }
                }
                /* ok we may need to convert to some decent untis */
                strncpy(local_unit, cmor_vars[var_id].ounits, CMOR_MAX_STRING);

                cmor_units = ut_parse(ut_read, local_unit, UT_ASCII);

                if (ut_get_status() != UT_SUCCESS) {
                    cmor_handle_error_var_variadic(
                        "Udunits: Error parsing units: %s, for zaxis %s, variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        local_unit, cmor_axes[axis_id].id,
                        cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }

                strncpy(local_unit, units, CMOR_MAX_STRING);
                ut_trim(local_unit, UT_ASCII);

                user_units = ut_parse(ut_read, local_unit, UT_ASCII);
                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_var_variadic(
                        "Udunits: Error parsing units: %s, zaxis %s, variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        local_unit, cmor_axes[axis_id].id,
                        cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }
                ut_cmor_converter = ut_get_converter(user_units, cmor_units);

                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_var_variadic(
                        "Udunits: Error getting converter from %s to %s, zaxis %s, variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        units, cmor_vars[var_id].ounits,
                        cmor_axes[axis_id].id, cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }
                cv_convert_doubles(ut_cmor_converter,
                                   &cmor_vars[var_id].values[0], 2 * n,
                                   &cmor_vars[var_id].values[0]);

                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_var_variadic(
                        "Udunits: Error converting units from %s to %s, zaxis %s, variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        units, cmor_vars[var_id].ounits,
                        cmor_axes[axis_id].id, cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }

                cv_free(ut_cmor_converter);

                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_var_variadic(
                        "Udunits: Error freeing converter, zaxis %s, "
                        "variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        cmor_axes[axis_id].id, cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }

                ut_free(cmor_units);

                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_var_variadic(
                        "Udunits: Error freeing cmor units %s, zaxis "
                        "%s, variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        cmor_vars[var_id].ounits,
                        cmor_axes[axis_id].id, cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }

                ut_free(user_units);

                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_var_variadic(
                        "Udunits: Error freeing units %s, zaxis %s, "
                        "variable %s (table: %s)",
                        CMOR_CRITICAL, var_id,
                        units, cmor_axes[axis_id].id,
                        cmor_vars[var_id].id,
                        cmor_tables[cmor_vars[var_id].ref_table_id].
                        szTable_id);

                }

            }
        }
    } else {
        /* first prepare the conversion thing */

        /* stores the original hybrid_in to put back later */
        i = cmor_axes[axis_id].hybrid_in;
        cmor_axes[axis_id].hybrid_in = cmor_axes[axis_id].hybrid_out;
        /* ok we now have 3 possible case */
        if (i == 1) {
            n = cmor_zfactor(zvar_id, axis_id, name, units, ndims,
                             axes_ids, type, values, bounds);
            /* case alternate hybrid sigma */
        } else if (i == 2) {

            if (strcmp(name, "ap") == 0) {
                /* creates the p0 */
                tmp = (double)1.e5;
                j = cmor_zfactor(zvar_id, axis_id, "p0", "Pa", 0, NULL,
                                 'd', &tmp, NULL);
                /* creates the "a" */

                n = cmor_zfactor(zvar_id, axis_id, "a", "", ndims, axes_ids, type, values, bounds);     /* ok redefined it as a "a" factor */

                /* ok we need to change the values now */
                /* first convert p0 to user units */
                cmor_units = ut_parse(ut_read, "Pa", UT_ASCII);
                strncpy(local_unit, units, CMOR_MAX_STRING);
                ut_trim(local_unit, UT_ASCII);
                user_units = ut_parse(ut_read, local_unit, UT_ASCII);

                if (ut_get_status() != UT_SUCCESS) {
                    cmor_handle_error_variadic(
                        "Udunits: Error parsing user units: %s, "
                        "zaxis %s (table: %s), when creating "
                        "zfactor: %s",
                        CMOR_CRITICAL,
                        local_unit, cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);
                }
                if (ut_are_convertible(cmor_units, user_units) == 0) {
                    cmor_handle_error_variadic(
                        "Udunuits: Pa and user units (%s) are "
                        "incompatible, zaxis %s (table: %s), when "
                        "creating zfactor: %s",
                        CMOR_CRITICAL,
                        units, cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);
                    cmor_pop_traceback();
                    return (1);
                }
                ut_cmor_converter = ut_get_converter(cmor_units, user_units);
                if (ut_get_status() != UT_SUCCESS) {
                    cmor_handle_error_variadic(
                        "Udunits: Error getting converter from Pa "
                        "to %s,variable %s (table %s), when creating "
                        "zfactor: %s",
                        CMOR_CRITICAL,
                        units, cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);
                }
                tmp = (double)1.e5;
                tmp = cv_convert_double(ut_cmor_converter, tmp);
                /* free units thing */
                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_variadic(
                        "Udunits: Error converting units from Pa "
                        "to %s, zaxis %s (table: %s), when creating "
                        "zfactor: %s",
                        CMOR_CRITICAL,
                        local_unit, cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);

                }
                cv_free(ut_cmor_converter);
                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_variadic(
                        "Udunits: Error freeing converter, zaxis %s "
                        "(table: %s), when creating zfactor: %s",
                        CMOR_CRITICAL,
                        cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);

                }

                ut_free(cmor_units);
                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_variadic(
                        "Udunits: Error freeing units Pa, zaxis: %s "
                        "(table: %s), when creating zfactor: %s",
                        CMOR_CRITICAL,
                        cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);
                }

                ut_free(user_units);
                if (ut_get_status() != UT_SUCCESS) {

                    cmor_handle_error_variadic(
                        "Udunits: Error freeing units %s, zaxis %s "
                        "(table: %s), when creating zfactor: %s",
                        CMOR_CRITICAL,
                        local_unit, cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);

                }

                if (values != NULL) {
                    n = cmor_axes[axes_ids[0]].length;
                    for (j = 0; j < n; j++)
                        cmor_vars[*zvar_id].values[j] /= tmp;
                }
                if (bounds != NULL) {

                    k = -1;
                    for (n = 0; n <= cmor_nvars; n++)
                        if ((strcmp(cmor_vars[n].id, "a_bnds") == 0)
                            && (cmor_vars[n].zaxis == axis_id)) {
                            k = n;
                            break;
                        }

                    n = cmor_axes[axes_ids[0]].length;
                    for (j = 0; j < 2 * n; j++)
                        cmor_vars[k].values[j] /= tmp;
                }
            } else {
                n = cmor_zfactor(zvar_id, axis_id, name, units, ndims,
                                 axes_ids, type, values, bounds);
            }
/* atmosphere_sigma_coordinates case */
        } else if (i == 3) {
            if (strcmp(name, "sigma") == 0) {

                /*ok first we need to make sure we created the ptop */
                j = -1;
                for (n = 0; n <= cmor_nvars; n++) {
                    if ((strcmp(cmor_vars[n].id, "ptop") == 0)
                        && (cmor_vars[n].zaxis == axis_id)) {
                        j = n;
                        break;
                    }
                }

                if (j == -1) {  /* we did not find the ztop! */
                    cmor_handle_error_variadic(
                        "zfactor variable \"ptop\" for zfactor axis: "
                        "%i (%s, table: %s), is not defined when "
                        "creating zfactor %s, please define ptop first",
                        CMOR_CRITICAL,
                        axis_id, cmor_axes[axis_id].id,
                        cmor_tables[cmor_axes[axis_id].ref_table_id].
                        szTable_id, name);
                }

                tmp = (double)1.e5;

                /* creates the p0 */

                n = cmor_zfactor(zvar_id, axis_id, "p0", "Pa", 0, NULL,
                                 'd', &tmp, NULL);

                tmp = cmor_vars[j].values[0];   /* stores ptop (in Pa) */

                /* creates the "a" */
                /* ok redefined it as a "a" factor */

                n = cmor_zfactor(zvar_id, axis_id, "a", "", ndims,
                                 axes_ids, type, values, bounds);

                if (values != NULL) {
                    n = cmor_axes[axes_ids[0]].length;
                    for (j = 0; j < n; j++)
                        cmor_vars[*zvar_id].values[j] =
                          (1. - cmor_vars[*zvar_id].values[j]) * tmp / 1.e5;
                }

                if (bounds != NULL) {
                    k = -1;
                    for (n = 0; n <= cmor_nvars; n++)
                        if ((strcmp(cmor_vars[n].id, "a_bnds") == 0)
                            && (cmor_vars[n].zaxis == axis_id)) {
                            k = n;
                            break;
                        }

                    n = cmor_axes[axes_ids[0]].length;
                    for (j = 0; j < 2 * n; j++)
                        cmor_vars[k].values[j] /= tmp;
                }

                /* creates the "b" */
                n = cmor_zfactor(zvar_id, axis_id, "b", "", ndims, axes_ids, type, values, bounds);     /* ok redefined it as a "a" factor */
            } else
                n = cmor_zfactor(zvar_id, axis_id, name, units, ndims,
                                 axes_ids, type, values, bounds);
        }

        /* put back input type */
        cmor_axes[axis_id].hybrid_in = i;

    }
    cmor_pop_traceback();
    return (stop);
}

/************************************************************************/
/*                        cmor_update_history()                         */
/************************************************************************/
int cmor_update_history(int var_id, char *add)
{
    struct tm *ptr;
    time_t lt;
    char date[CMOR_MAX_STRING];
    char tmp[CMOR_MAX_STRING];
    char tmp2[CMOR_MAX_STRING];

    /* first figures out time */
    lt = time(NULL);
    ptr = gmtime(&lt);
    snprintf(date, CMOR_MAX_STRING, "%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ",
             ptr->tm_year + 1900, ptr->tm_mon + 1, ptr->tm_mday, ptr->tm_hour,
             ptr->tm_min, ptr->tm_sec);

    if (cmor_has_variable_attribute(var_id, "history") == 0) {

        cmor_get_variable_attribute(var_id, "history", &tmp[0]);

    } else {

        tmp[0] = '\0';
    }

    snprintf(tmp2, CMOR_MAX_STRING, "%s %s altered by CMOR: %s.",
             tmp, date, add);

    cmor_set_variable_attribute_internal(var_id, "history", 'c', tmp2);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_history_contains()                        */
/************************************************************************/

int cmor_history_contains(int var_id, char *add)
{
    char tmp[CMOR_MAX_STRING];

    if (cmor_has_variable_attribute(var_id, "history") == 0) {
        cmor_get_variable_attribute(var_id, "history", &tmp[0]);
        if (cmor_stringinstring(tmp, add)) {
            return (1);
        }
    }
    return (0);
}

/************************************************************************/
/*                           cmor_variable()                            */
/************************************************************************/
int cmor_variable(int *var_id, char *name, char *units, int ndims,
                  int axes_ids[], char type, void *missing,
                  double *tolerance, char *positive, char *original_name,
                  char *history, char *comment)
{

    extern int cmor_nvars, cmor_naxes;
    extern int CMOR_TABLE;
    extern cmor_var_t cmor_vars[];

    int i, iref, j, k, l;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    cmor_var_def_t refvar;
    cmor_CV_def_t *frequencies_cv, *freq_cv, 
    *interv_cv, *interv_warn_cv, *interv_err_cv;
    int laxes_ids[CMOR_MAX_DIMENSIONS];
    int grid_id = 1000;
    int lndims, olndims;
    float afloat;
    int aint;
    long along;
    int did_grid_reorder = 0;
    int vrid = -1;

    cmor_add_traceback("cmor_variable");
    cmor_is_setup();

    if (CMOR_TABLE == -1) {
        cmor_handle_error("You did not define a table yet!", CMOR_CRITICAL);
    }

    if (cmor_nvars == CMOR_MAX_VARIABLES - 1) {
        cmor_handle_error("Too many variables defined", CMOR_CRITICAL);
        cmor_pop_traceback();
        return (1);
    }

/* -------------------------------------------------------------------- */
/*      ok now look which variable is corresponding in table if not     */
/*      found then error                                                */
/* -------------------------------------------------------------------- */
    iref = -1;
    cmor_trim_string(name, ctmp);
    if ((comment != NULL) && strcmp(comment, COMMENT_VARIABLE_ZFACTOR) == 0) {
        for (i = 0; i < cmor_tables[CMOR_TABLE].nformula + 1; i++) {
            if (strcmp(cmor_tables[CMOR_TABLE].formula[i].id, ctmp) == 0) {
                iref = i + CMOR_MAX_ELEMENTS;
                break;
            }
        }

    } else {
        for (i = 0; i < cmor_tables[CMOR_TABLE].nvars + 1; i++) {
            if (strcmp(cmor_tables[CMOR_TABLE].vars[i].id, ctmp) == 0) {
                iref = i;
                break;
            }
        }
    }

    if (iref == -1) {
        cmor_handle_error_variadic(
            "Could not find a matching variable for name: '%s'", CMOR_CRITICAL, ctmp);
    }

    if (iref > CMOR_MAX_ELEMENTS) {
        refvar = cmor_tables[CMOR_TABLE].formula[iref - CMOR_MAX_ELEMENTS];
    } else {
        refvar = cmor_tables[CMOR_TABLE].vars[iref];

    }
    for (i = 0; i < CMOR_MAX_VARIABLES; i++) {
        if (cmor_vars[i].self == -1) {
            vrid = i;
            break;
        }
    }

    if (vrid > cmor_nvars)
        cmor_nvars = vrid;

    cmor_vars[vrid].ref_table_id = CMOR_TABLE;
    cmor_vars[vrid].ref_var_id = iref;

/* -------------------------------------------------------------------- */
/*      init some things                                                */
/* -------------------------------------------------------------------- */

    strcpy(cmor_vars[vrid].suffix, "");
    strcpy(cmor_vars[vrid].base_path, "");
    strcpy(cmor_vars[vrid].current_path, "");

    // If the frequency was selected by the user (either in the user
    // input JSON or using the `cmor_set_cur_dataset_attribute` function to
    // set the `cv_frequency` attribute) then get the frequency from the
    // current dataset's `cv_frequency` attribute. Otherwise, get the
    // frequency from the variable definition from the current table.
    if (strcmp(cmor_current_dataset.cv_frequency, "") != 0) {
        strncpy(cmor_vars[vrid].frequency, cmor_current_dataset.cv_frequency,
            CMOR_MAX_STRING);
    } else if (refvar.frequency[0] != '\0') {
        strncpy(cmor_vars[vrid].frequency, refvar.frequency, CMOR_MAX_STRING);
    }

    cmor_vars[vrid].suffix_has_date = 0;

/* -------------------------------------------------------------------- */
/*      output missing value                                            */
/* -------------------------------------------------------------------- */
    if (refvar.type == 'd') {
        cmor_vars[vrid].omissing = (double)cmor_tables[CMOR_TABLE].missing_value;

    } else if (refvar.type == 'f') {
        cmor_vars[vrid].omissing = (double)cmor_tables[CMOR_TABLE].missing_value;

    } else if (refvar.type == 'l') {
        cmor_vars[vrid].omissing = (double)cmor_tables[CMOR_TABLE].int_missing_value;

    } else if (refvar.type == 'i') {
        cmor_vars[vrid].omissing = (double)cmor_tables[CMOR_TABLE].int_missing_value;
    }


/* -------------------------------------------------------------------- */
/*      copying over values from ref var                                */
/* -------------------------------------------------------------------- */

    cmor_vars[vrid].valid_min = refvar.valid_min;
    cmor_vars[vrid].valid_max = refvar.valid_max;
    cmor_vars[vrid].ok_min_mean_abs = refvar.ok_min_mean_abs;
    cmor_vars[vrid].ok_max_mean_abs = refvar.ok_max_mean_abs;
    cmor_vars[vrid].shuffle = refvar.shuffle;
    cmor_vars[vrid].deflate = refvar.deflate;
    cmor_vars[vrid].deflate_level = refvar.deflate_level;
    cmor_vars[vrid].zstandard_level = refvar.zstandard_level;
    strcpy(cmor_vars[vrid].chunking_dimensions, refvar.chunking_dimensions);

    cmor_vars[vrid].quantize_mode = NC_NOQUANTIZE;
    cmor_vars[vrid].quantize_nsd = 1;

    if (refvar.out_name[0] == '\0') {
        strncpy(cmor_vars[vrid].id, name, CMOR_MAX_STRING);
    } else {
        strncpy(cmor_vars[vrid].id, refvar.out_name, CMOR_MAX_STRING);
    }

    strncpy(cmor_vars[vrid].branding_suffix, refvar.branding_suffix, CMOR_MAX_STRING);
    strncpy(cmor_vars[vrid].temporal_label, refvar.temporal_label, CMOR_MAX_STRING);
    strncpy(cmor_vars[vrid].vertical_label, refvar.vertical_label, CMOR_MAX_STRING);
    strncpy(cmor_vars[vrid].horizontal_label, refvar.horizontal_label, CMOR_MAX_STRING);
    strncpy(cmor_vars[vrid].area_label, refvar.area_label, CMOR_MAX_STRING);

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_STANDARDNAME, 'c',
                                         refvar.standard_name);

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_LONGNAME, 'c',
                                         refvar.long_name);

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_VARTITLE, 'c',
                                         refvar.variable_title);

    if (refvar.flag_values[0] != '\0') {
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGVALUES, 'c',
                                             refvar.flag_values);
    }
    if (refvar.flag_meanings[0] != '\0') {

        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGMEANINGS,
                                             'c', refvar.flag_meanings);
    }

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_COMMENT, 'c',
                                         refvar.comment);

    if (strcmp(refvar.units, "?") == 0) {
        strncpy(cmor_vars[vrid].ounits, units, CMOR_MAX_STRING);
    } else {
        strncpy(cmor_vars[vrid].ounits, refvar.units, CMOR_MAX_STRING);
    }

    if (refvar.type != 'c') {
        cmor_set_variable_attribute_internal(vrid,
                                             VARIABLE_ATT_UNITS,
                                             'c', cmor_vars[vrid].ounits);
    }

    strncpy(cmor_vars[vrid].iunits, units, CMOR_MAX_STRING);

    if ((original_name != NULL) && (original_name[0] != '\0')) {
        cmor_set_variable_attribute_internal(vrid,
                                             VARIABLE_ATT_ORIGINALNAME,
                                             'c', original_name);
    }

    if ((history != NULL) && (history[0] != '\0')) {
        cmor_set_variable_attribute_internal(vrid,
                                             VARIABLE_ATT_HISTORY,
                                             'c', history);
    }

    if ((comment != NULL) && (comment[0] != '\0') &&
        strcmp(comment, COMMENT_VARIABLE_ZFACTOR) != 0) {
        if (cmor_has_variable_attribute(vrid, VARIABLE_ATT_COMMENT) == 0) {
            char szActivity[CMOR_MAX_STRING];

            cmor_get_cur_dataset_attribute(GLOBAL_ATT_ACTIVITY_ID, szActivity);

            strncpy(msg, comment, CMOR_MAX_STRING);
            strncat(msg, ", ", CMOR_MAX_STRING - strlen(msg));
            strncat(msg, szActivity, CMOR_MAX_STRING - strlen(msg));
            strncat(msg, "_table_comment: ", CMOR_MAX_STRING - strlen(msg));
            strncat(msg, refvar.comment, CMOR_MAX_STRING - strlen(msg));

        } else {
            strncpy(msg, comment, CMOR_MAX_STRING);
        }
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_COMMENT, 'c',
                                             msg);
    }

    if (strcmp(units, refvar.units) != 0) {
        cmor_set_variable_attribute_internal(vrid,
                                             VARIABLE_ATT_ORIGINALUNITS,
                                             'c', units);
        snprintf(msg,
                 CMOR_MAX_STRING,
                 "Converted units from '%s' to '%s'", units, refvar.units);
        cmor_update_history(vrid, msg);
    }

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_CELLMETHODS,
                                         'c', refvar.cell_methods);

    cmor_set_variable_attribute_internal(vrid,
                                         VARIABLE_ATT_CELLMEASURES,
                                         'c', refvar.cell_measures);

    if ((positive != NULL) && (positive[0] != '\0')) {
        if ((positive[0] != 'd') && positive[0] != 'u') {
            cmor_handle_error_var_variadic(
                "variable '%s' (table %s): unknown value for "
                "positive : %s (only first character is considered, "
                "which was: %c)",
                CMOR_CRITICAL, vrid,
                cmor_vars[vrid].id,
                cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                positive, positive[0]);
        }

        if (refvar.positive == 'u') {
            if (cmor_is_required_variable_attribute(refvar,
                                                    VARIABLE_ATT_POSITIVE) ==
                0) {

                cmor_set_variable_attribute_internal(vrid,
                                                     VARIABLE_ATT_POSITIVE, 'c',
                                                     "up");

            }

            if (positive[0] != 'u') {
                cmor_vars[vrid].sign = -1;
                cmor_update_history(vrid, "Changed sign");
            }

        } else if (refvar.positive == 'd') {
            if (cmor_is_required_variable_attribute(refvar,
                                                    VARIABLE_ATT_POSITIVE) ==
                0) {

                cmor_set_variable_attribute_internal(vrid,
                                                     VARIABLE_ATT_POSITIVE, 'c',
                                                     "down");

            }
            if (positive[0] != 'd') {
                cmor_vars[vrid].sign = -1;
                cmor_update_history(vrid, "Changed sign");
            }
        } else {
            cmor_handle_error_variadic(
                "variable '%s' (table %s) you passed positive "
                "value:%s, but table does not mention it, will "
                "be ignored, if you really want this in your "
                "variable output use "
                "cmor_set_variable_attribute_internal function",
                CMOR_WARNING,
                cmor_vars[vrid].id,
                cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                positive);
        }
    } else {
        if (cmor_is_required_variable_attribute(refvar, VARIABLE_ATT_POSITIVE)
            == 0) {
            cmor_handle_error_var_variadic(
                "you need to provide the 'positive' argument for "
                "variable: %s (table %s)",
                CMOR_CRITICAL, vrid,
                cmor_vars[vrid].id,
                cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id);
        }
        if (refvar.positive != '\0') {
            if (refvar.positive == 'u') {
                if (cmor_is_required_variable_attribute(refvar,
                                                        VARIABLE_ATT_POSITIVE)
                    == 0) {
                    cmor_set_variable_attribute_internal(vrid,
                                                         VARIABLE_ATT_POSITIVE,
                                                         'c', "up");
                }

                cmor_handle_error_var_variadic(
                    "you did not provide the 'positive' argument "
                    "for variable: %s (table %s)",
                    CMOR_CRITICAL, vrid,
                    cmor_vars[vrid].id,
                    cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id);

            } else if (refvar.positive == 'd') {

                if (cmor_is_required_variable_attribute(refvar,
                                                        VARIABLE_ATT_POSITIVE)
                    == 0) {
                    cmor_set_variable_attribute_internal(vrid,
                                                         VARIABLE_ATT_POSITIVE,
                                                         'c', "down");
                }
                cmor_handle_error_var_variadic(
                    "you did not provide the 'positive' argument for variable: %s (table %s)",
                    CMOR_CRITICAL, vrid,
                    cmor_vars[vrid].id,
                    cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id);
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      before anything we copy axes_ids into laxes_ids                 */
/* -------------------------------------------------------------------- */

    for (i = 0; i < ndims; i++) {
        laxes_ids[i] = axes_ids[i];
    }
/* -------------------------------------------------------------------- */
/*      Now figure out if the variable ask for an axis that is          */
/*      actually calling for a grid to be defined                       */
/* -------------------------------------------------------------------- */

    k = 0;
    for (i = 0; i < refvar.ndims; i++) {

        for (j = 0; j < cmor_tables[cmor_vars[vrid].ref_table_id].naxes; j++) {

            if (strcmp
                (cmor_tables[cmor_vars[vrid].ref_table_id].axes
                 [refvar.dimensions[i]].id,
                 cmor_tables[cmor_vars[vrid].ref_table_id].axes[j].id) == 0) {

                if (cmor_tables[cmor_vars[vrid].ref_table_id].axes
                    [refvar.dimensions[i]].must_call_cmor_grid == 1)
                    k = 1;
                break;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      ok we MUST HAVE called cmor_grid to generate this variable      */
/*      let's make sure                                                 */
/* -------------------------------------------------------------------- */
    if (k == 1) {
        j = 0;
        for (i = 0; i < ndims; i++) {
            if (laxes_ids[i] < -CMOR_MAX_GRIDS + 1) {
                grid_id = -laxes_ids[i] - CMOR_MAX_GRIDS;
                if (grid_id > cmor_ngrids)
                    continue;
                j = 1;
            }
        }
        if (j == 0) {
            cmor_handle_error_var_variadic(
            "Variable %s (table %s) must be defined using a "
            "grid (a call to cmor_grid)",
            CMOR_CRITICAL, vrid,
            cmor_vars[vrid].id,
            cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id);
        }
    }

    lndims = ndims;

/* -------------------------------------------------------------------- */
/*      just to know if we deal with  a grid                            */
/* -------------------------------------------------------------------- */
    aint = 0;

/* -------------------------------------------------------------------- */
/*      ok we need to replace grids definitions with the grid axes      */
/* -------------------------------------------------------------------- */
    for (i = 0; i < ndims; i++) {
        if (laxes_ids[i] > cmor_naxes) {
            cmor_handle_error_var_variadic(
                "For variable %s (table %s) you requested axis_id "
                "(%i) that has not been defined yet",
                CMOR_CRITICAL, vrid,
                cmor_vars[vrid].id,
                cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                laxes_ids[i]);
        }
        if (laxes_ids[i] < -CMOR_MAX_GRIDS + 1) {
            grid_id = -laxes_ids[i] - CMOR_MAX_GRIDS;
            if (grid_id > cmor_ngrids) {
                cmor_handle_error_var_variadic(
                    "For variable %s (table: %s) you requested "
                    "grid_id (%i) that has not been defined yet",
                    CMOR_CRITICAL, vrid,
                    cmor_vars[vrid].id,
                    cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                    laxes_ids[i]);
            }
/* -------------------------------------------------------------------- */
/*      here we need to know if the refvar has been defined with        */
/*      lat/lon or in the grid space                                    */
/* -------------------------------------------------------------------- */
            k = 0;
            for (j = 0; j < refvar.ndims; j++) {
                if (strcmp
                    (cmor_tables[refvar.table_id].axes[refvar.dimensions[j]].id,
                     DIMENSION_LONGITUDE) == 0)
                    k++;
                if (strcmp
                    (cmor_tables[refvar.table_id].axes[refvar.dimensions[j]].id,
                     DIMENSION_LATITUDE) == 0)
                    k++;
                if (refvar.dimensions[j] == -CMOR_MAX_GRIDS)
                    k++;
            }
/* -------------------------------------------------------------------- */
/*      basically replaces the lat/lon with the number of dims in       */
/*      our grid                                                        */
/* -------------------------------------------------------------------- */

            if (k == 2) {
                aint = cmor_grids[grid_id].ndims - 2;
            }
            cmor_vars[vrid].grid_id = grid_id;
            k = cmor_grids[grid_id].ndims - 1;

/* -------------------------------------------------------------------- */
/*      first move everything to the right                              */
/* -------------------------------------------------------------------- */

            for (j = lndims - 1; j >= i; j--)
                laxes_ids[j + k] = laxes_ids[j];
/* -------------------------------------------------------------------- */
/*      ok now we need to insert the grid dimensions                    */
/* -------------------------------------------------------------------- */

            lndims += k;
            for (j = 0; j < cmor_grids[grid_id].ndims; j++) {
                laxes_ids[i + j] = cmor_grids[grid_id].original_axes_ids[j];
            }
        }
    }
    olndims = lndims;
    if (refvar.ndims + aint != lndims) {
        lndims = 0;
/* -------------------------------------------------------------------- */
/*      ok before we panic we check if there is a "dummy" dim           */
/* -------------------------------------------------------------------- */

        j = refvar.ndims - olndims + aint;
        for (i = 0; i < refvar.ndims; i++) {
            if (refvar.dimensions[i] >= 0
                && cmor_tables[CMOR_TABLE].axes[refvar.dimensions[i]].value
                != 1.e20) {
/* -------------------------------------------------------------------- */
/*      ok it could be a dummy but we need to check if the user         */
/*      already defined this dummy dimension or notd                    */
/* -------------------------------------------------------------------- */

                l = -1;
                for (k = 0; k < olndims; k++) {
                    strncpytrim(msg, 
                                cmor_tables[cmor_axes[laxes_ids[k]].ref_table_id]
                                .axes[cmor_axes[laxes_ids[k]].ref_axis_id].id, 
                                CMOR_MAX_STRING);

                    if (strcmp(msg,
                               cmor_tables[CMOR_TABLE].
                               axes[refvar.dimensions[i]].id)
                        == 0) {
/* -------------------------------------------------------------------- */
/*       ok user did define this one on its own                         */
/* -------------------------------------------------------------------- */
                        l = k;
                        break;
                    }

                }
/* -------------------------------------------------------------------- */
/*      ok it is a singleton dimension                                  */
/* -------------------------------------------------------------------- */

                if (l == -1) {
                    j -= 1;
/* -------------------------------------------------------------------- */
/*      ok then we create a dummy axis that we will add at the end      */
/*      of the axes                                                     */
/* -------------------------------------------------------------------- */
                    cmor_axis_def_t *pAxis;
                    pAxis = &cmor_tables[CMOR_TABLE].axes[refvar.dimensions[i]];

                    if (pAxis->bounds_value[0] != 1.e20) {

                        cmor_axis(&k,
                                  pAxis->id,
                                  pAxis->units,
                                  1,
                                  &pAxis->value,
                                  'd', &pAxis->bounds_value[0], 2, "");

                    } else {

                        cmor_axis(&k,
                                  pAxis->id,
                                  pAxis->units,
                                  1, &pAxis->value, 'd', NULL, 0, "");
                    }

                    laxes_ids[olndims + lndims] = k;
                    lndims += 1;
                }

            }
        }

        if ((j != 0) && (j != -1)) {
            cmor_handle_error_var_variadic(
                "You are defining variable '%s' (table %s) with %i "
                "dimensions, when it should have %i",
                CMOR_CRITICAL, vrid,
                name,
                cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                ndims, refvar.ndims);
            cmor_pop_traceback();
            return (1);
        } else {
            lndims += ndims;
            for (j = 0; j < ndims; j++) {
/* -------------------------------------------------------------------- */
/*      grid definition                                                 */
/* -------------------------------------------------------------------- */

                if (axes_ids[j] < -CMOR_MAX_GRIDS + 1) {
                    lndims += cmor_grids[grid_id].ndims - 1;
                }
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      At that point we need to check that the dims we passed match    */
/*      what's in the refvar                                            */
/* -------------------------------------------------------------------- */

    k = -1;
    for (i = 0; i < lndims; i++) {
        if (strcmp
            (cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
             [cmor_axes[laxes_ids[i]].ref_axis_id].id,
             AXIS_FORECAST_TIME) == 0) {
            refvar.dimensions[lndims - 1] = cmor_axes[laxes_ids[i]].ref_axis_id;
            refvar.ndims++;
            k++;
            continue;
        }
        for (j = 0; j < refvar.ndims; j++) {
            if (strcmp
                 (cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                  [cmor_axes[laxes_ids[i]].ref_axis_id].id,
                  cmor_tables[CMOR_TABLE].axes[refvar.dimensions[j]].id) == 0) {
                      k++;
                  }
                else if ((cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                  [cmor_axes[laxes_ids[i]].ref_axis_id].axis == 'Z')
                 && (refvar.dimensions[j] == -2)) {
                k++;
/* ok it is a olvel or similar (refvar == -2) */
/* we need to ensure it is the correct one */
                if (
                    (strcmp(refvar.generic_level_name,
                cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                [cmor_axes[laxes_ids[i]].ref_axis_id].generic_level_name) != 0) && 
                (cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                     [cmor_axes[laxes_ids[i]].ref_axis_id].generic_level_name[0] != '\0')
                 ) {
                    cmor_handle_error_var_variadic(
                        "You defined variable '%s' (table %s) with axis "
                        "id '%s', the variable calls for a generic axis of type '%s' "
                        "according to your table, the axis you are providing is of generic type '%s'",
                        CMOR_CRITICAL, vrid,
                        refvar.id,
                        cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                        cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].
                        axes[cmor_axes[laxes_ids[i]].ref_axis_id].id,
                        refvar.generic_level_name,
                        cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                        [cmor_axes[laxes_ids[i]].ref_axis_id].generic_level_name
                        );
                }
            }
        }
        if (k != i) {
/* -------------------------------------------------------------------- */
/*      ok we didn't find it, but there is still a slight chance it     */
/*      is a grid axis!                                                 */
/* -------------------------------------------------------------------- */

            for (j = 0; j < cmor_grids[grid_id].ndims; j++) {
                if (laxes_ids[i] == cmor_grids[grid_id].original_axes_ids[j])
                    k++;
            }
        }
        if (k != i) {
            snprintf(msg, CMOR_MAX_STRING,
                     "You defined variable '%s' (table %s) with axis "
                     "id '%s' which is not part of this variable "
                     "according to your table, it says: ( ",
                     refvar.id,
                     cmor_tables[cmor_vars[vrid].ref_table_id].szTable_id,
                     cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].
                     axes[cmor_axes[laxes_ids[i]].ref_axis_id].id);
            for (i = 0; i < refvar.ndims; i++) {
                strcat(msg,
                       cmor_tables[CMOR_TABLE].axes[refvar.dimensions[i]].id);
                strcat(msg, " ");
            }
            strcat(msg, ")");
            cmor_handle_error_var(msg, CMOR_CRITICAL, vrid);
        }
    }
    k = 0;
/* -------------------------------------------------------------------- */
/*      ok now loop thru axes                                           */
/* -------------------------------------------------------------------- */

    for (i = 0; i < lndims; i++) {

        if (laxes_ids[i] > cmor_naxes) {
            cmor_handle_error_var_variadic("Axis %i not defined", CMOR_CRITICAL, vrid, axes_ids[i]);
            cmor_pop_traceback();
            return (1);
        }
        if (cmor_axes[laxes_ids[i]].type == 'T') {
            if (strcmp(cmor_axes[laxes_ids[i]].cv_frequency, "") != 0) {
                if (strcmp(cmor_axes[laxes_ids[i]].cv_frequency, cmor_vars[vrid].frequency) != 0) {
                    cmor_handle_error_var_variadic(
                        "The variable definition frequency is \"%s\" but the time axis has the "
                        "user-defined frequency \"%s\"",
                        CMOR_CRITICAL, vrid,
                        cmor_vars[vrid].frequency, cmor_axes[laxes_ids[i]].cv_frequency
                    );
                } else {
                    frequencies_cv = cmor_CV_rootsearch(cmor_tables[cmor_axes[cmor_naxes].
                        ref_table_id].CV, CV_KEY_FREQUENCY);
                    if(frequencies_cv != NULL) {
                        freq_cv = cmor_CV_search_child_key(frequencies_cv, cmor_vars[vrid].frequency);
                        if(freq_cv != NULL) {
                            interv_cv = cmor_CV_search_child_key(freq_cv, CV_KEY_APRX_INTRVL);
                            interv_err_cv = cmor_CV_search_child_key(freq_cv, CV_KEY_APRX_INTRVL_ERR);
                            interv_warn_cv = cmor_CV_search_child_key(freq_cv, CV_KEY_APRX_INTRVL_WRN);
                            if((interv_cv != NULL && interv_cv->dValue != cmor_axes[laxes_ids[i]].approx_interval)
                            || (interv_err_cv != NULL && interv_err_cv->dValue != cmor_axes[laxes_ids[i]].approx_interval_error)
                            || (interv_warn_cv != NULL && interv_warn_cv->dValue != cmor_axes[laxes_ids[i]].approx_interval_warning)){
                                cmor_handle_error_var_variadic(
                                    "The interval values used for the time axis differ "
                                    "from those defined in the CV for frequency \"%s\"",
                                    CMOR_CRITICAL, vrid, cmor_vars[vrid].frequency
                                );
                            }
                        }
                    }
                }
            } else if (strcmp(cmor_current_dataset.cv_frequency, "") != 0) {
                cmor_handle_error_var_variadic(
                    "The current user-defined frequency is \"%s\" but the time axis does not "
                    "use this frequency",
                    CMOR_CRITICAL, vrid,
                    cmor_current_dataset.cv_frequency
                );
            }
        }
        if (cmor_axes[laxes_ids[i]].ref_table_id != CMOR_TABLE
            && cmor_axes[laxes_ids[i]].isgridaxis != 1) {
            cmor_handle_error_var_variadic(
                "While creating variable %s, you are "
                "passing axis %i (named %s) which has been "
                "defined using table %i (%s) but the current "
                "table is %i (%s) (and isgridaxis says: %i)",
                CMOR_CRITICAL, vrid,
                cmor_vars[vrid].id, laxes_ids[i],
                cmor_axes[laxes_ids[i]].id,
                cmor_axes[laxes_ids[i]].ref_table_id,
                cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].
                szTable_id, CMOR_TABLE, cmor_tables[CMOR_TABLE].szTable_id,
                cmor_axes[laxes_ids[i]].isgridaxis);
        }
        if (cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
            [cmor_axes[laxes_ids[i]].ref_axis_id].value != 1.e20) {

/* -------------------------------------------------------------------- */
/*      singleton dim                                                   */
/* -------------------------------------------------------------------- */

            snprintf(msg, CMOR_MAX_STRING,
                     "Treated scalar dimension: '%s'",
                     cmor_axes[laxes_ids[i]].id);
            cmor_update_history(vrid, msg);
            cmor_vars[vrid].singleton_ids[i - k] = laxes_ids[i];
            if (cmor_has_variable_attribute(vrid, "coordinates") == 0) {
                cmor_get_variable_attribute(vrid, "coordinates", &msg[0]);
            } else {
                strncpy(msg, "", CMOR_MAX_STRING);
            }
            if (cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                [cmor_axes[laxes_ids[i]].ref_axis_id].out_name[0] == '\0') {
                snprintf(ctmp, CMOR_MAX_STRING, "%s %s", msg,
                         cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                         [cmor_axes[laxes_ids[i]].ref_axis_id].id);
            } else {
                snprintf(ctmp, CMOR_MAX_STRING, "%s %s", msg,
                         cmor_tables[cmor_axes[laxes_ids[i]].ref_table_id].axes
                         [cmor_axes[laxes_ids[i]].ref_axis_id].out_name);
            }
            cmor_set_variable_attribute_internal(vrid,
                                                 VARIABLE_ATT_COORDINATES, 'c',
                                                 &ctmp[0]);

        } else {

            cmor_vars[vrid].original_order[k] = laxes_ids[i];
            k++;
        }
    }
/* -------------------------------------------------------------------- */
/*      Now figures out the real order...                               */
/* -------------------------------------------------------------------- */

    k = 0;

    for (i = 0; i < lndims; i++) {
        char *name = refvar.dimensions[i] >= 0
            ? cmor_tables[refvar.table_id].axes[refvar.dimensions[i]].id
            : "";

        if ((strcmp(name, "latitude") == 0 || strcmp(name, "longitude") == 0)
            && grid_id != 1000) {
/* -------------------------------------------------------------------- */
/*      ok we are  dealing with a "grid" type of data                   */
/* -------------------------------------------------------------------- */

            if (did_grid_reorder != 0)
                continue;
            for (j = 0; j < cmor_grids[grid_id].ndims; j++) {
                cmor_vars[vrid].axes_ids[k] = cmor_grids[grid_id].axes_ids[j];
                k++;
            }
            did_grid_reorder = 1;
        } else if ((refvar.dimensions[i] == -2)
                   || (cmor_tables[CMOR_TABLE].axes[refvar.dimensions[i]].value
                       == 1.e20)) {
/* -------------------------------------------------------------------- */
/*      not a singleton dim                                             */
/* -------------------------------------------------------------------- */

            iref = -1;
            for (j = 0; j < lndims; j++) {
                if ((refvar.table_id == cmor_axes[laxes_ids[j]].ref_table_id)
                    && (refvar.dimensions[i]
                        == cmor_axes[laxes_ids[j]].ref_axis_id)) {
                    cmor_vars[vrid].axes_ids[k] = laxes_ids[j];
                }
/* -------------------------------------------------------------------- */
/*      -2 means it is a zaxis                                          */
/* -------------------------------------------------------------------- */

                if (refvar.dimensions[i] == -2) {
                    if (cmor_axes[laxes_ids[j]].axis == 'Z')
                        cmor_vars[vrid].axes_ids[k] = laxes_ids[j];
                }
            }
            k++;
        } else if (refvar.dimensions[i] == -CMOR_MAX_GRIDS) {
/* -------------------------------------------------------------------- */
/*      ok this is either a lat/lon                                     */
/* -------------------------------------------------------------------- */

            for (j = 0; j < ndims; j++)
                if (axes_ids[j] < -CMOR_MAX_GRIDS + 1)
                    break;
            l = j;
            for (j = 0; j < cmor_grids[grid_id].ndims; j++) {
                cmor_vars[vrid].axes_ids[k] = cmor_grids[grid_id].axes_ids[j];
                k++;
                i++;
            }
            i--;                /* one too many i adds */
        }
    }

/* -------------------------------------------------------------------- */
/*      OK WE ARE SAYING THAT THIS VARIABLE HAS %i DIMENSIONS           */
/* -------------------------------------------------------------------- */

    if ( strcmp(cmor_vars[vrid].id, "vertices_longitude") == 0 && k == 1 )
      { k++;
      cmor_vars[vrid].axes_ids[1] = axes_ids[1]; }
    else if ( strcmp(cmor_vars[vrid].id, "vertices_latitude") == 0 && k == 1 )
      { k++;
      cmor_vars[vrid].axes_ids[1] = axes_ids[1]; }

    cmor_vars[vrid].ndims = k;
    cmor_vars[vrid].itype = type;
    k = 0;

    for (i = 0; i < cmor_vars[vrid].ndims; i++)
        if (cmor_vars[vrid].original_order[i] != cmor_vars[vrid].axes_ids[i])
            k = -1;
    if (k == -1) {

        strncpy(msg, "Reordered dimensions, original order:", CMOR_MAX_STRING);
        for (i = 0; i < cmor_vars[vrid].ndims; i++) {
            snprintf(ctmp, CMOR_MAX_STRING, " %s",
                     cmor_axes[cmor_vars[vrid].original_order[i]].id);

            strncat(msg, ctmp, CMOR_MAX_STRING - strlen(ctmp));
        }

        cmor_update_history(vrid, msg);
    }
/* -------------------------------------------------------------------- */
/*      Set Missing Value                                               */
/* -------------------------------------------------------------------- */

    if (refvar.type == '\0') {
        cmor_vars[vrid].type = 'f';
    } else {
        cmor_vars[vrid].type = refvar.type;
    }

    if (missing != NULL) {
        cmor_vars[vrid].nomissing = 0;
        if (type == 'i')
            cmor_vars[vrid].missing = (double)*(int *)missing;
        if (type == 'l')
            cmor_vars[vrid].missing = (double)*(long *)missing;
        if (type == 'f')
            cmor_vars[vrid].missing = (double)*(float *)missing;
        if (type == 'd')
            cmor_vars[vrid].missing = (double)*(double *)missing;
        if (fabs((cmor_vars[vrid].missing/cmor_vars[vrid].omissing)-1) > 0.001) {
            snprintf(msg, CMOR_MAX_STRING,
                     "replaced missing value flag (%g) and "
                     "corresponding data with standard missing value (%g)",
                     cmor_vars[vrid].missing, cmor_vars[vrid].omissing);
            cmor_update_history(vrid, msg);
        }
    }
    if( cmor_vars[vrid].ref_var_id < CMOR_MAX_ELEMENTS  ){
        if (refvar.type == 'd') {
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_MISSINGVALUES, 'd', &cmor_vars[vrid].omissing);
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_FILLVAL, 'd', &cmor_vars[vrid].omissing);

        } else if (refvar.type == 'f') {

            afloat = (float) cmor_vars[vrid].omissing;
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_MISSINGVALUES, 'f', &afloat);
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_FILLVAL, 'f', &afloat);
        } else if (refvar.type == 'l') {

            along = (long) cmor_vars[vrid].omissing;
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_MISSINGVALUES, 'l', &along);
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_FILLVAL, 'l', &along);

        } else if (refvar.type == 'i') {

            aint = (int) cmor_vars[vrid].omissing;
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_MISSINGVALUES, 'i', &aint);
            cmor_set_variable_attribute_internal(vrid,
            VARIABLE_ATT_FILLVAL, 'i', &aint);
        }
    }
    cmor_vars[vrid].tolerance = 1.e-4;
    cmor_vars[vrid].self = vrid;
    if (tolerance != NULL)
        cmor_vars[vrid].tolerance = (double)*(double *)tolerance;
    *var_id = vrid;
    cmor_pop_traceback();
    return (0);
};

/************************************************************************/
/*                         cmor_init_var_def()                          */
/************************************************************************/

void cmor_init_var_def(cmor_var_def_t * var, int table_id)
{
    int n;

    cmor_is_setup();
    var->table_id = table_id;
    var->standard_name[0] = '\0';
    var->variable_title[0] = '\0';
    var->units[0] = '\0';
    var->cell_methods[0] = '\0';
    var->cell_measures[0] = '\0';
    var->positive = cmor_tables[table_id].positive;
    var->long_name[0] = '\0';
    var->comment[0] = '\0';
    var->realm[0] = '\0';
    var->frequency[0] = '\0';
    var->out_name[0] = '\0';
    var->ndims = 0;
    var->flag_values[0] = '\0';
    var->flag_meanings[0] = '\0';
    var->chunking_dimensions[0] = '\0';
    for (n = 0; n < CMOR_MAX_DIMENSIONS; n++)
        var->dimensions[n] = -1;
    var->type = cmor_tables[table_id].type;
    var->valid_min = cmor_tables[table_id].valid_min;     /* means no check */
    var->valid_max = cmor_tables[table_id].valid_max;
    var->ok_min_mean_abs = cmor_tables[table_id].ok_min_mean_abs;
    var->ok_max_mean_abs = cmor_tables[table_id].ok_max_mean_abs;
    var->shuffle = 0;
    var->deflate = 1;
    var->deflate_level = 1;
    var->zstandard_level = 3;
    var->generic_level_name[0] = '\0';
    var->branding_suffix[0] = '\0';
    var->temporal_label[0] = '\0';
    var->vertical_label[0] = '\0';
    var->horizontal_label[0] = '\0';
    var->area_label[0] = '\0';
}

/************************************************************************/
/*                        cmor_set_var_def_att()                        */
/************************************************************************/

int cmor_set_var_def_att(cmor_var_def_t * var, char *att, char *val)
{
    int i, n, j, n0, k;
    char dim[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_set_var_def_att");
    cmor_is_setup();
    if (strcmp(val, "") == 0) {
        cmor_pop_traceback();
        return (0);
    }
    if (strcmp(att, VARIABLE_ATT_REQUIRED) == 0) {

        strncpy(var->required, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_ID) == 0) {

        strncpy(var->id, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_STANDARDNAME) == 0) {

        strncpy(var->standard_name, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_LONGNAME) == 0) {

        strncpy(var->long_name, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_COMMENT) == 0) {

        strncpy(var->comment, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_DIMENSIONS) == 0) {
        strncpy(var->dimensions_str, val, CMOR_MAX_STRING);
        n0 = strlen(val);
        for (i = 0; i < n0; i++) {

            j = 0;
            while ((i < n0) && ((val[i] != ' ') && val[i] != '\0')) {
                dim[j] = val[i];
                j++;
                i++;
            }

            dim[j] = '\0';

            if (var->ndims > CMOR_MAX_DIMENSIONS) {
                cmor_handle_error_variadic(
                    "Too many dimensions (%i) defined for variable "
                    "(%s), max is: %i",
                    CMOR_CRITICAL,
                    var->ndims, var->id,
                    CMOR_MAX_DIMENSIONS);
            }

/* -------------------------------------------------------------------- */
/*      check that the dimension as been defined in the table           */
/* -------------------------------------------------------------------- */
            n = -1;             /* not found yet */
            for (j = 0; j <= cmor_tables[var->table_id].naxes; j++) {
                if (strcmp(dim, cmor_tables[var->table_id].axes[j].id) == 0) {
                    n = j;
                    break;
                }
            }
            if (strcmp(AXIS_FORECAST_LEADTIME, cmor_tables[var->table_id].axes[n].forecast) == 0) {
                /* this is a leadtime coordinate, we want to skip it at this point as it doesn't really count against
                 dimensionality of the variable, and is not present in the input data. This coordinate will be
                 inserted back as the final step of the variable creation */
                continue;
            }

            if (n == -1) {
                j = strcmp(DIMENSION_ZLEVEL, dim);
                j *= strcmp(DIMENSION_ALEVEL, dim);
                j *= strcmp(DIMENSION_ALEVEL_HALF, dim);
                j *= strcmp(DIMENSION_OLEVEL, dim);
                j *= strcmp(DIMENSION_OLEVEL_HALF, dim);
                for (k = 0; k < CMOR_MAX_ELEMENTS; k++) {
                    if (cmor_tables[var->table_id].generic_levels[k][0] == '\0')
                        break;
                    j *= strcmp(dim,
                                cmor_tables[var->table_id].generic_levels[k]);
                }

                if (j == 0) {

                    var->dimensions[var->ndims] = -2;
                    strncpy(var->generic_level_name, dim, CMOR_MAX_STRING);

                } else {
                    if ((strcmp(dim, DIMENSION_LONGITUDE) != 0)
                        && strcmp(dim, DIMENSION_LATITUDE) != 0) {
/* -------------------------------------------------------------------- */
/*      do not raise a warning if the axis is "longitude" /             */
/*      "latitude" it is probably a grid variable                       */
/* -------------------------------------------------------------------- */

                        cmor_handle_error_variadic(
                            "Reading table %s: axis name: '%s' for "
                            "variable: '%s' is not defined in table. "
                            "Table defines dimensions: '%s' for this "
                            "variable",
                            CMOR_CRITICAL,
                            cmor_tables[var->table_id].szTable_id, dim,
                            var->id, val);
                    } else {
                        var->dimensions[var->ndims] = -CMOR_MAX_GRIDS;
                    }
                }
            } else {
                var->dimensions[var->ndims] = n;
            }
            var->ndims++;
        }
/* -------------------------------------------------------------------- */
/*      revert to put in C order                                        */
/* -------------------------------------------------------------------- */

        for (i = 0; i < var->ndims / 2; i++) {
            n = var->dimensions[i];
            var->dimensions[i] = var->dimensions[var->ndims - 1 - i];
            var->dimensions[var->ndims - 1 - i] = n;
        }

    } else if (strcmp(att, VARIABLE_ATT_UNITS) == 0) {

        strncpy(var->units, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_CELLMETHODS) == 0) {

        strncpy(var->cell_methods, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_EXTCELLMEASURES) == 0) {

        strncpy(var->cell_measures, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_CELLMEASURES) == 0) {

        strncpy(var->cell_measures, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_POSITIVE) == 0) {

        var->positive = val[0];

    } else if (strcmp(att, VARIABLE_ATT_TYPE) == 0) {

        if (strcmp(val, "real") == 0)
            var->type = 'f';

        else if (strcmp(val, "double") == 0)
            var->type = 'd';

        else if (strcmp(val, "integer") == 0)
            var->type = 'i';

        else if (strcmp(val, "long") == 0)
            var->type = 'l';

    } else if (strcmp(att, VARIABLE_ATT_VALIDMIN) == 0) {

        var->valid_min = atof(val);

    } else if (strcmp(att, VARIABLE_ATT_VALIDMAX) == 0) {

        var->valid_max = atof(val);

    } else if (strcmp(att, VARIABLE_ATT_MINMEANABS) == 0) {

        var->ok_min_mean_abs = atof(val);

    } else if (strcmp(att, VARIABLE_ATT_MAXMEANABS) == 0) {

        var->ok_max_mean_abs = atof(val);
    } else if (strcmp(att, VARIABLE_ATT_CHUNKING) == 0) {
        strncpy(var->chunking_dimensions, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_SHUFFLE) == 0) {

        var->shuffle = atoi(val);

        if (atoi(val) != 0) {

            if (USE_NETCDF_4 == 0) {
                cmor_handle_error_variadic(
                    "Reading a table (%s) that calls for NetCDF4 "
                    "features, you are using NetCDF3 library",
                    CMOR_WARNING,
                    cmor_tables[var->table_id].szTable_id);

            } else if ((CMOR_NETCDF_MODE == CMOR_APPEND_3) ||
                       (CMOR_NETCDF_MODE == CMOR_REPLACE_3) ||
                       (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)) {

                cmor_handle_error_variadic(
                    "Reading a table (%s) that calls for NetCDF4 "
                    "features, you asked for NetCDF3 features",
                    CMOR_WARNING,
                    cmor_tables[var->table_id].szTable_id);
            }
        }
    } else if (strcmp(att, VARIABLE_ATT_DEFLATE) == 0) {

        var->deflate = atoi(val);

        if (atoi(val) != 0) {
            if (USE_NETCDF_4 == 0) {
                cmor_handle_error_variadic(
                    "Reading a table (%s) that calls for NetCDF4 features, you are using NetCDF3 library",
                    CMOR_WARNING,
                    cmor_tables[var->table_id].szTable_id);
            } else if ((CMOR_NETCDF_MODE == CMOR_APPEND_3) ||
                       (CMOR_NETCDF_MODE == CMOR_REPLACE_3) ||
                       (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)) {
                cmor_handle_error_variadic(
                    "Reading a table (%s) that calls for NetCDF4 features, you asked for NetCDF3 features",
                    CMOR_WARNING,
                    cmor_tables[var->table_id].szTable_id);
            }
        }
    } else if (strcmp(att, VARIABLE_ATT_DEFLATELEVEL) == 0) {

        var->deflate_level = atoi(val);

    } else if (strcmp(att, VARIABLE_ATT_ZSTANDARDLEVEL) == 0) {

        var->zstandard_level = atoi(val);

    } else if (strcmp(att, VARIABLE_ATT_MODELINGREALM) == 0) {

        strncpy(var->realm, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIALBE_ATT_FREQUENCY) == 0) {

        strncpy(var->frequency, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_FLAGVALUES) == 0) {

        strncpy(var->flag_values, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_FLAGMEANINGS) == 0) {

        strncpy(var->flag_meanings, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_OUTNAME) == 0) {

        strncpy(var->out_name, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_BRANDINGSUFFIX) == 0) {

        strncpy(var->branding_suffix, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_TEMPORALLABEL) == 0) {

        strncpy(var->temporal_label, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_VERTICALLABEL) == 0) {

        strncpy(var->vertical_label, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_HORIZONTALLABEL) == 0) {

        strncpy(var->horizontal_label, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_AREALABEL) == 0) {

        strncpy(var->area_label, val, CMOR_MAX_STRING);

    } else if (strcmp(att, VARIABLE_ATT_VARTITLE) == 0) {

        strncpy(var->variable_title, val, CMOR_MAX_STRING);

    } else {
        cmor_handle_error_variadic(
            "Table %s, unknown variable attribute: >>>>%s<<<< value: (%s)",
            CMOR_WARNING,
            cmor_tables[var->table_id].szTable_id, att, val);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                      cmor_set_var_chunking()                         */
/************************************************************************/
int cmor_set_chunking(int var_id, int nTableID, size_t nc_dim_chunking[])
{

    char chunk_dimensions[CMOR_MAX_STRING];
    char *token;
    int n;
    int ndims = cmor_vars[var_id].ndims;
    int nChunks[CMOR_MAX_DIMENSIONS];   // T, Z, Y,X
    int nAxisID;

    cmor_add_traceback("cmor_set_chunking");
    cmor_is_setup();

    strcpy(chunk_dimensions, cmor_vars[var_id].chunking_dimensions);
    if (chunk_dimensions[0] == '\0') {
        cmor_pop_traceback();
        return (-1);
    }

    token = strtok(chunk_dimensions, " ");
    n = 0;
    // Read in all chunks
    while (token != NULL) {
        nChunks[n] = atoi(token);
        n++;
        token = strtok(NULL, " ");
    }
    // We need 4 dimensions corresponding to T, Z, Y,X
    if (n != 4) {
        return (-1);
    }
    // Validate Chunks size.
    for (n = 0; n < ndims; n++) {
        nAxisID = cmor_vars[var_id].axes_ids[n];
        if (cmor_axes[nAxisID].axis == 'X') {
            if (nChunks[3] > cmor_axes[nAxisID].length) {
                nChunks[3] = cmor_axes[nAxisID].length;
            } else if (nChunks[3] <= 0) {
                nChunks[3] = 1;
            }
        }
        if (cmor_axes[nAxisID].axis == 'Y') {
            if (nChunks[2] > cmor_axes[nAxisID].length) {
                nChunks[2] = cmor_axes[nAxisID].length;
            } else if (nChunks[2] <= 0) {
                nChunks[2] = 1;
            }
        }
        if (cmor_axes[nAxisID].axis == 'Z') {
            if (nChunks[1] > cmor_axes[nAxisID].length) {
                nChunks[1] = cmor_axes[nAxisID].length;
            } else if (nChunks[1] <= 0) {
                nChunks[1] = 1;
            }
        }
        if (cmor_axes[nAxisID].axis == 'T') {
            if (nChunks[0] > cmor_axes[nAxisID].length) {
                nChunks[0] = cmor_axes[nAxisID].length;
            } else if (nChunks[0] <= 0) {
                nChunks[0] = 1;
            }
        }
    }
    // Assign chunks;
    n = 0;
    while (n < ndims) {
        nAxisID = cmor_vars[var_id].axes_ids[n];
        if (cmor_axes[nAxisID].axis == 'X') {
            nc_dim_chunking[n] = nChunks[3];
        } else if (cmor_axes[nAxisID].axis == 'Y') {
            nc_dim_chunking[n] = nChunks[2];
        } else if (cmor_axes[nAxisID].axis == 'Z') {
            nc_dim_chunking[n] = nChunks[1];
        } else if (cmor_axes[nAxisID].axis == 'T') {
            nc_dim_chunking[n] = nChunks[0];
        } else {
            nc_dim_chunking[n] = 1;
        }
        n++;
    }
    cmor_pop_traceback();
    return (0);

}

/************************************************************************/
/*                       cmor_set_var_deflate()                         */
/************************************************************************/
int cmor_set_deflate(int var_id, int shuffle, int deflate, int deflate_level)
{
    cmor_add_traceback("cmor_get_original_shape");
    cmor_is_setup();

    if (cmor_vars[var_id].self != var_id) {
        cmor_handle_error_var_variadic(
            "You attempt to deflate variable id(%d) which was "
            "not initialized",
            CMOR_CRITICAL, var_id,
            var_id);
        cmor_pop_traceback();

        return (-1);
    }

    cmor_vars[var_id].shuffle = shuffle;
    cmor_vars[var_id].deflate = deflate;
    cmor_vars[var_id].deflate_level = deflate_level;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_set_zstandard()                           */
/************************************************************************/
int cmor_set_zstandard(int var_id, int zstandard_level)
{
    cmor_add_traceback("cmor_set_zstandard");
    cmor_is_setup();

    if (cmor_vars[var_id].self != var_id) {
        cmor_handle_error_var_variadic(
            "You attempted to set the zstandard level of "
            "variable id(%d) which was not initialized",
            CMOR_CRITICAL, var_id,
            var_id);
        cmor_pop_traceback();

        return (-1);
    }

    cmor_vars[var_id].zstandard_level = zstandard_level;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_set_quantize()                            */
/************************************************************************/
int cmor_set_quantize(int var_id, int quantize_mode, int quantize_nsd)
{
    cmor_add_traceback("cmor_set_quantize");
    cmor_is_setup();

    if (cmor_vars[var_id].self != var_id) {
        cmor_handle_error_var_variadic(
            "You attempted to set the quantize mode of "
            "variable id(%d) which was not initialized",
            CMOR_CRITICAL, var_id,
            var_id);
        cmor_pop_traceback();

        return (-1);
    }

    cmor_vars[var_id].quantize_mode = quantize_mode;
    cmor_vars[var_id].quantize_nsd = quantize_nsd;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                   cmor_get_variable_time_length()                    */
/************************************************************************/
int cmor_get_variable_time_length(int *var_id, int *length)
{
    cmor_var_t avar;
    int i;

    *length = 0;
    avar = cmor_vars[*var_id];
    for (i = 0; i < avar.ndims; i++) {
        if (cmor_axes[avar.original_order[i]].axis == 'T')
            *length = cmor_axes[avar.original_order[i]].length;
    }

    return (0);
}

/************************************************************************/
/*                      cmor_get_original_shape()                       */
/************************************************************************/

int cmor_get_original_shape(int *var_id, int *shape_array, int *rank,
                            int blank_time)
{
    int i;
    cmor_var_t avar;

    cmor_add_traceback("cmor_get_original_shape");
    avar = cmor_vars[*var_id];
    for (i = 0; i < *rank; i++)
        shape_array[i] = -1;    /* init array */

    if (*rank < avar.ndims) {
        cmor_handle_error_var_variadic(
            "trying to retrieve shape of variable %s (table: %s) into a %id "
            "array but this variable is %id",
            CMOR_CRITICAL, *var_id,
            avar.id, cmor_tables[avar.ref_table_id].szTable_id, *rank,
            avar.ndims);
    }
    for (i = 0; i < avar.ndims; i++) {
        if ((blank_time == 1)
            && (cmor_axes[avar.original_order[i]].axis == 'T')) {
            shape_array[i] = 0;
        } else {
            shape_array[i] = cmor_axes[avar.original_order[i]].length;
        }
    }

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_write_var_to_file()                       */
/************************************************************************/
int cmor_write_var_to_file(int ncid, cmor_var_t * avar, void *data,
                           char itype, int ntimes_passed,
                           double *time_vals, double *time_bounds)
{

    size_t counts[CMOR_MAX_DIMENSIONS];
    size_t counts2[CMOR_MAX_DIMENSIONS];
    size_t counter[CMOR_MAX_DIMENSIONS];
    size_t counter_orig[CMOR_MAX_DIMENSIONS];
    size_t counter_orig2[CMOR_MAX_DIMENSIONS];
    size_t counter2[CMOR_MAX_DIMENSIONS];
    size_t max_counter[CMOR_MAX_DIMENSIONS];
    size_t min_counter[CMOR_MAX_DIMENSIONS];
    size_t starts[CMOR_MAX_DIMENSIONS];
    size_t nelements, loc, add, nelts;
    double *data_tmp = NULL, tmp = 0., tmp2, amean;
    int *idata_tmp = NULL;
    long *ldata_tmp = NULL;
    float *fdata_tmp = NULL;
    char mtype;
    size_t i, j;
    int ierr = 0, dounits = 1;
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];
    double *tmp_vals;
    ut_unit *user_units = NULL, *cmor_units = NULL;
    cv_converter *ut_cmor_converter = NULL;
    char local_unit[CMOR_MAX_STRING];
    size_t n_lower_min = 0, n_greater_max = 0;
    double emax, emin, first_time;
    extern ut_system *ut_read;
    size_t tmpindex = 0;
    size_t index;
    char *msg_min;
    char *msg_max;
    size_t msg_len;

    cmor_add_traceback("cmor_write_var_to_file");
    cmor_is_setup();

    emax = 0.;
    emin = 0.;

    if (strcmp(avar->ounits, avar->iunits) == 0)
        dounits = 0;
    mtype = avar->type;
/* -------------------------------------------------------------------- */
/*       This counts how many elements there is in each dimension and   */
/*      the total number of elements written at this time This needs    */
/*      to be passed to NetCDF.                                         */
/*                                                                      */
/*      do we have times ?                                              */
/* -------------------------------------------------------------------- */
    if (ntimes_passed != 0) {
        counts[0] = ntimes_passed;
        if (cmor_axes[avar->axes_ids[0]].axis != 'T') {
            cmor_handle_error_variadic(
                "you are passing %i time steps for a static "
                "(no time dimension) variable (%s, table: %s), "
                "please pass 0 (zero) as the number of times",
                CMOR_CRITICAL,
                ntimes_passed, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
        }
    } else {
/* -------------------------------------------------------------------- */
/*      need to determine if it is a static variable                    */
/* -------------------------------------------------------------------- */

        if (avar->ndims > 0)
            counts[0] = cmor_axes[avar->axes_ids[0]].length;
        else
            counts[0] = 1;
    }
    nelements = counts[0];
    for (i = 1; i < avar->ndims; i++) {
        counts[i] = cmor_axes[avar->axes_ids[i]].length;
        nelements = nelements * counts[i];
    }
    if (avar->isbounds == 1)
        nelements *= 2;
/* -------------------------------------------------------------------- */
/*      This section counts how many elements are needed before you     */
/*      increase the index in each dimension                            */
/* -------------------------------------------------------------------- */

    counter[avar->ndims] = 1;   /* dummy */
    counter_orig[avar->ndims] = 1;      /*dummy */

    for (i = avar->ndims; i > 0; i--) {
/* -------------------------------------------------------------------- */
/*      we need to do this for the order in which we will write and     */
/*      the order the user defined its variable                         */
/* -------------------------------------------------------------------- */
        if (cmor_axes[avar->axes_ids[i - 1]].axis != 'T')
            counter[i - 1] = cmor_axes[avar->axes_ids[i - 1]].length * counter[i];
        else
            counter[i - 1] = counts[0] * counter[i];
        if (cmor_axes[avar->original_order[i - 1]].axis != 'T')
            counter_orig[i - 1] = cmor_axes[avar->original_order[i - 1]].length
              * counter_orig[i];
        else
            counter_orig[i - 1] = counts[0] * counter_orig[i];
    }
/* -------------------------------------------------------------------- */
/*       Now we need to map, i.e going ahead by 2 elements of final     */
/*       array eq going ahead of n elements originally                 */
/* -------------------------------------------------------------------- */

    for (i = 0; i < avar->ndims; i++) {
        for (j = 0; j < avar->ndims; j++) {
            if (avar->axes_ids[i] == avar->original_order[j]) {
                index = j + 1;
                counter_orig2[i] = counter_orig[index];
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Allocates the memory to store data to be written after          */
/*      reordering and scaling/off-setting needs to figure out if we    */
/*      need to touch the variable...                                   */
/* -------------------------------------------------------------------- */
    if (mtype == 'i') {
        idata_tmp = malloc(sizeof(int) * nelements);
        if (idata_tmp == NULL) {
            cmor_handle_error_variadic(
                "cannot allocate memory for %lu int tmp elts var '%s' "
                "(table: %s)",
                CMOR_CRITICAL,
                nelements, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
        }

    } else if (mtype == 'l') {

        ldata_tmp = malloc(sizeof(long) * nelements);
        if (ldata_tmp == NULL) {
            cmor_handle_error_variadic(
                "cannot allocate memory for %lu long tmp elts var '%s' "
                "(table: %s)",
                CMOR_CRITICAL,
                nelements, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
        }

    } else if (mtype == 'd') {

        data_tmp = malloc(sizeof(double) * nelements);
        if (data_tmp == NULL) {
            cmor_handle_error_variadic(
                "cannot allocate memory for %lu double tmp elts var '%s' "
                "(table: %s)",
                CMOR_CRITICAL,
                nelements, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
        }

    } else {

        fdata_tmp = malloc(sizeof(float) * nelements);
        if (fdata_tmp == NULL) {
            cmor_handle_error_variadic(
                "cannot allocate memory for %lu float tmp elts var '%s' "
                "(table: %s)",
                CMOR_CRITICAL,
                nelements, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
        }
    }

/* -------------------------------------------------------------------- */
/*      Reorder data, applies scaling, etc...                           */
/* -------------------------------------------------------------------- */
    if (dounits == 1) {

        strncpy(local_unit, avar->ounits, CMOR_MAX_STRING);
        ut_trim(local_unit, UT_ASCII);
        cmor_units = ut_parse(ut_read, local_unit, UT_ASCII);

        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                "in udunits analyzing units from cmor table "
                "(%s) for variable %s (table: %s)",
                CMOR_CRITICAL,
                local_unit, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }

        strncpy(local_unit, avar->iunits, CMOR_MAX_STRING);
        ut_trim(local_unit, UT_ASCII);
        user_units = ut_parse(ut_read, local_unit, UT_ASCII);

        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                "in udunits analyzing units from user (%s) "
                "for variable %s (table: %s)",
                CMOR_CRITICAL,
                local_unit, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }

        if (ut_are_convertible(cmor_units, user_units) == 0) {
            cmor_handle_error_variadic(
                "variable: %s, cmor and user units are incompatible: "
                "%s and %s for variable %s (table: %s)",
                CMOR_CRITICAL,
                avar->id, avar->ounits, avar->iunits, avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }

        ut_cmor_converter = ut_get_converter(user_units, cmor_units);

        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                " in udunits, getting converter for variable %s "
                "(table: %s)",
                CMOR_CRITICAL,
                avar->id, cmor_tables[avar->ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (1);
        }
    }

    amean = 0.;
    nelts = 0;

    for (i = 0; i < nelements; i++) {
        loc = i;
/* -------------------------------------------------------------------- */
/*      first figures out the coeff in final order                      */
/*      puts the result in counter2                                     */
/* -------------------------------------------------------------------- */
        for (j = 0; j < avar->ndims; j++) {
            counter2[j] = (size_t)loc / (size_t)counter[j + 1];

/* -------------------------------------------------------------------- */
/*      this is the reverse part doing it this way to avoid if test     */
/* -------------------------------------------------------------------- */
            loc = loc - counter2[j] * counter[j + 1];
        }

/* -------------------------------------------------------------------- */
/*      now figures out what these indices meant in the original order  */
/* -------------------------------------------------------------------- */
        loc = 0;
        for (j = 0; j < avar->ndims; j++) {
            cmor_axis_t *pAxis;
            pAxis = &cmor_axes[avar->axes_ids[j]];
            if (pAxis->axis != 'T') {

                add = counter2[j] * pAxis->revert + (pAxis->length - 1) *
                  (1 - pAxis->revert) / 2;

                loc = loc + (size_t)fmod(add + pAxis->offset, pAxis->length)
                  * counter_orig2[j];

            } else {

                add = counter2[j] * pAxis->revert + (counts[0] - 1) *
                  (1 - pAxis->revert) / 2;
                loc = loc + (size_t)fmod(add + pAxis->offset, counts[0])
                  * counter_orig2[j];
            }

        }

/* -------------------------------------------------------------------- */
/*      Copy from user's data into our data                             */
/* -------------------------------------------------------------------- */
        if (itype == 'd')
            tmp = (double)((double *)data)[loc];
        else if (itype == 'f')
            tmp = (double)((float *)data)[loc];
        else if (itype == 'i')
            tmp = (double)((int *)data)[loc];
        else if (itype == 'l')
            tmp = (double)((long *)data)[loc];
        if (avar->isbounds) {

/* -------------------------------------------------------------------- */
/*      ok here's the code to flip the code if necessary                */
/* -------------------------------------------------------------------- */
            if (cmor_axes[avar->axes_ids[0]].revert == -1) {
                loc = nelements - i - 1;
            } else {
                loc = i;
            }
            tmp = (double)((double *)data)[loc];
        }

        tmp2 = (double)fabs(tmp - avar->missing);

        if ((avar->nomissing == 0)
            && (tmp2 <= avar->tolerance * (double)fabs(tmp))) {
            tmp = avar->omissing;

        } else {
            if (dounits == 1) {

                tmp = cv_convert_double(ut_cmor_converter, tmp);

                if (ut_get_status() != UT_SUCCESS) {
                    cmor_handle_error_variadic(
                        "in udunits, converting values from %s to %s "
                        "for variable %s (table: %s)",
                        CMOR_CRITICAL,
                        avar->iunits, avar->ounits, avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                    cmor_pop_traceback();
                    return (1);
                }
            }

            tmp = tmp * avar->sign;     /* do we need to change the sign ? */
            amean += fabs(tmp);
            nelts += 1;

            if ((avar->valid_min != (float)1.e20) && (tmp < avar->valid_min)) {

                n_lower_min += 1;
                if ((n_lower_min == 1) || (tmp < emin)) {       /*minimum val */
                    emin = tmp;
                    for (j = 0; j < avar->ndims; j++) {
                        min_counter[j] = counter2[j];
                    }
                }
            }
            if ((avar->valid_max != (float)1.e20) && (tmp > avar->valid_max)) {

                n_greater_max += 1;
                if ((n_greater_max == 1) || (tmp > emax)) {
                    emax = tmp;
                    for (j = 0; j < avar->ndims; j++) {
                        max_counter[j] = counter2[j];
                    }
                }
            }
        }

        if (mtype == 'i')
            idata_tmp[i] = (int)tmp;
        else if (mtype == 'l')
            ldata_tmp[i] = (long)tmp;
        else if (mtype == 'f')
            fdata_tmp[i] = (float)tmp;
        else if (mtype == 'd')
            data_tmp[i] = (double)tmp;

    }
    if (n_lower_min != 0) {
        msg_len = 0;
        for (j = 0; j < avar->ndims; j++) {
            cmor_axis_t *pAxis;
            pAxis = &cmor_axes[avar->axes_ids[j]];
            double val = 0.f;
            if (pAxis->values != NULL) {
                val = pAxis->values[min_counter[j]];
            } else {
                val = time_vals[min_counter[j]];
            }
            msg_len += snprintf(NULL, 0, " %s: %lu/%.5g",
                                pAxis->id, min_counter[j], val);
        }
        msg_len += 1;

        msg_min = (char *)malloc(msg_len * sizeof(char));
        
        msg_len = 0;
        for (j = 0; j < avar->ndims; j++) {
            cmor_axis_t *pAxis;
            pAxis = &cmor_axes[avar->axes_ids[j]];
            double val = 0.f;
            if (pAxis->values != NULL) {
                val = pAxis->values[min_counter[j]];
            } else {
                val = time_vals[min_counter[j]];
            }
            msg_len += sprintf(&msg_min[msg_len], " %s: %lu/%.5g",
                    pAxis->id, min_counter[j], val);
        }
    
        cmor_handle_error_variadic(
            "Invalid value(s) detected for variable '%s' "
            "(table: %s): %zu values were lower than minimum "
            "valid value (%.4g). Minimum encountered bad "
            "value (%.5g) was at (axis: index/value):%s",
            CMOR_WARNING,
            avar->id,
            cmor_tables[avar->ref_table_id].szTable_id,
            n_lower_min, avar->valid_max, emin, msg_min);

        free(msg_min);

    }
    if (n_greater_max != 0) {
        msg_len = 0;
        for (j = 0; j < avar->ndims; j++) {
            cmor_axis_t *pAxis;
            pAxis = &cmor_axes[avar->axes_ids[j]];
            double val = 0.;
            if (pAxis->values != NULL) {
                val = pAxis->values[max_counter[j]];
            } else {
                val = time_vals[max_counter[j]];
            }
            msg_len += snprintf(NULL, 0, " %s: %lu/%.5g",
                                pAxis->id, max_counter[j], val);
        }
        msg_len += 1;

        msg_max = (char *)malloc(msg_len * sizeof(char));

        msg_len = 0;
        for (j = 0; j < avar->ndims; j++) {
            cmor_axis_t *pAxis;
            pAxis = &cmor_axes[avar->axes_ids[j]];
            double val = 0.;
            if (pAxis->values != NULL) {
                val = pAxis->values[max_counter[j]];
            } else {
                val = time_vals[max_counter[j]];
            }
            msg_len += sprintf(&msg_max[msg_len], " %s: %lu/%.5g",
                    pAxis->id, max_counter[j], val);
        }
    
        cmor_handle_error_variadic(
            "Invalid value(s) detected for variable '%s' "
            "(table: %s): %zu values were greater than "
            "maximum valid value (%.4g).Maximum encountered "
            "bad value (%.5g) was at (axis: index/value):%s",
            CMOR_WARNING,
            avar->id,
            cmor_tables[avar->ref_table_id].szTable_id,
            n_greater_max, avar->valid_max, emax, msg_max);

        free(msg_max);

    }
    if (avar->ok_min_mean_abs != (float)1.e20) {

        if (amean / nelts < .1 * avar->ok_min_mean_abs) {

            cmor_handle_error_variadic(
                "Invalid Absolute Mean for variable '%s' (table: %s) "
                "(%.5g) is lower by more than an order of magnitude "
                "than minimum allowed: %.4g",
                CMOR_CRITICAL,
                avar->id,
                cmor_tables[avar->ref_table_id].szTable_id, amean / nelts,
                avar->ok_min_mean_abs);

        }
        if (amean / nelts < avar->ok_min_mean_abs) {

            cmor_handle_error_variadic(
                "Invalid Absolute Mean for variable '%s' "
                "(table: %s) (%.5g) is lower than minimum allowed: %.4g",
                CMOR_WARNING,
                avar->id, cmor_tables[avar->ref_table_id].szTable_id,
                amean / nelts, avar->ok_min_mean_abs);
        }
    }

    if (avar->ok_max_mean_abs != (float)1.e20) {
        if (amean / nelts > 10. * avar->ok_max_mean_abs) {
            cmor_handle_error_variadic(
                "Invalid Absolute Mean for variable '%s' "
                "(table: %s) (%.5g) is greater by more than "
                "an order of magnitude than maximum allowed: %.4g",
                CMOR_CRITICAL,
                avar->id, cmor_tables[avar->ref_table_id].szTable_id,
                amean / nelts, avar->ok_max_mean_abs);
        }
        if (amean / nelts > avar->ok_max_mean_abs) {

            cmor_handle_error_variadic(
                "Invalid Absolute Mean for variable '%s' "
                "(table: %s) (%.5g) is greater than maximum "
                "allowed: %.4g",
                CMOR_WARNING,
                avar->id,
                cmor_tables[avar->ref_table_id].szTable_id, amean / nelts,
                avar->ok_max_mean_abs);

        }
    }
    if (dounits == 1) {

        cv_free(ut_cmor_converter);

        if (ut_get_status() != UT_SUCCESS) {

            cmor_handle_error_variadic(
                "Udunits: Error freeing converter, variable %s "
                "(table: %s)",
                CMOR_CRITICAL,
                avar->id,
                cmor_tables[avar->ref_table_id].szTable_id);

        }

        ut_free(cmor_units);
        if (ut_get_status() != UT_SUCCESS) {

            cmor_handle_error_variadic(
                "Udunits: Error freeing units, variable %s (table: %s)",
                CMOR_CRITICAL,
                avar->id, cmor_tables[avar->ref_table_id].szTable_id);

        }

        ut_free(user_units);
        if (ut_get_status() != UT_SUCCESS) {

            cmor_handle_error_variadic(
                "Udunits: Error freeing units, variable %s (table: %s)",
                CMOR_CRITICAL,
                avar->id, cmor_tables[avar->ref_table_id].szTable_id);

        }
    }
/* -------------------------------------------------------------------- */
/*      Initialize the start index in each dimensions                   */
/* -------------------------------------------------------------------- */

    for (i = 0; i < avar->ndims; i++)
        starts[i] = 0;
    starts[0] = avar->ntimes_written;

/* -------------------------------------------------------------------- */
/*      Write the times passed by user                                  */
/* -------------------------------------------------------------------- */

    if (ntimes_passed != 0) {
        if (time_vals != NULL) {
            if (cmor_axes[avar->axes_ids[0]].values != NULL) {
                cmor_handle_error_variadic(
                    "variable '%s' (table %s) you are passing "
                    "time values but you already defined them "
                    "via cmor_axis, this is not allowed",
                    CMOR_CRITICAL,
                    avar->id,
                    cmor_tables[avar->ref_table_id].szTable_id);
            }

            if (time_bounds != NULL) {
                counts2[0] = counts[0];
                counts2[1] = 2;
                starts[1] = 0;
                cmor_get_axis_attribute(avar->axes_ids[0], "units", 'c', &msg);
                cmor_get_cur_dataset_attribute("calendar", msg2);

                tmp_vals = malloc((ntimes_passed + 1) * 2 * sizeof(double));
                if (tmp_vals == NULL) {
                    cmor_handle_error_variadic(
                        "cannot malloc %i tmp bounds time vals "
                        "for variable '%s' (table: %s)",
                        CMOR_CRITICAL,
                        ntimes_passed * 2, avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                }
                if (avar->ntimes_written > 0) {
                    if ((avar->last_time != -999.)
                        && (avar->last_bound != 1.e20)) {
                        tmpindex = 1;
                        tmp_vals[0] = avar->last_time;
                    } else {
                        tmpindex = 0;
                    }
                } else {
                    tmpindex = 0;
                }
                ierr = cmor_convert_time_values(time_vals, 'd', ntimes_passed,
                                                &tmp_vals[tmpindex],
                                                cmor_axes[avar->
                                                          axes_ids[0]].iunits,
                                                msg, msg2, msg2);

                ierr = cmor_check_monotonic(&tmp_vals[0],
                                            ntimes_passed + tmpindex, "time", 0,
                                            avar->axes_ids[0]);

                if (avar->ntimes_written > 0) {

                    if ((avar->last_time != -999.)
                        && (avar->last_bound != 1.e20)) {

                        tmp_vals[0] = 2 * avar->last_time - avar->last_bound;
                        tmp_vals[1] = avar->last_bound;
                    }
                }

                ierr = cmor_convert_time_values(time_bounds, 'd',
                                                ntimes_passed * 2,
                                                &tmp_vals[2 * tmpindex],
                                                cmor_axes[avar->
                                                          axes_ids[0]].iunits,
                                                msg, msg2, msg2);

                        ierr = cmor_check_monotonic(&tmp_vals[0],
                            (ntimes_passed + tmpindex) * 2,
                            "time", 1, avar->axes_ids[0]);

                ierr = cmor_check_values_inside_bounds(&time_vals[0],
                                                       &time_bounds[0],
                                                       ntimes_passed, "time");

                ierr = nc_put_vara_double(ncid, avar->time_bnds_nc_id, starts,
                                          counts2, &tmp_vals[2 * tmpindex]);

                if (ierr != NC_NOERR) {
                    cmor_handle_error_variadic(
                        "NetCDF error (%i) writing time bounds for variable '%s', already written in file: %i",
                        CMOR_CRITICAL,
                        ierr, avar->id, avar->ntimes_written);
                }
/* -------------------------------------------------------------------- */
/*      ok first time around the we need to store bounds                */
/* -------------------------------------------------------------------- */

                if (avar->ntimes_written == 0) {
/* -------------------------------------------------------------------- */
/*      Ok first time we're putting data  in                            */
/* -------------------------------------------------------------------- */

                    avar->first_bound = tmp_vals[0];
                } else {
/* -------------------------------------------------------------------- */
/*      ok let's put the bounds back on "normal" (start at 0) indices   */
/* -------------------------------------------------------------------- */

                    for (i = 0; i < 2 * ntimes_passed; i++) {
                        tmp_vals[i] = tmp_vals[i + 2];
                    }
                }
                avar->last_bound = tmp_vals[ntimes_passed * 2 - 1];

/* -------------------------------------------------------------------- */
/*      ok since we have bounds we need to set time in the middle       */
/*      but only do this in case of none climato                        */
/* -------------------------------------------------------------------- */
                if (cmor_tables[cmor_axes[avar->axes_ids[0]].ref_table_id].axes
                    [cmor_axes[avar->axes_ids[0]].ref_axis_id].climatology ==
                    0) {
                    for (i = 0; i < ntimes_passed; i++) {
                        tmp_vals[i] =
                          (tmp_vals[2 * i] + tmp_vals[2 * i + 1]) / 2.;
                    }
/* -------------------------------------------------------------------- */
/*      store for later                                                 */
/* -------------------------------------------------------------------- */

                    first_time = tmp_vals[0];
                } else {
/* -------------------------------------------------------------------- */
/*      we need to put into tmp_vals the right things                   */
/* -------------------------------------------------------------------- */
                    ierr = cmor_convert_time_values(time_vals, 'd',
                                                    ntimes_passed, &tmp_vals[0],
                                                    cmor_axes[avar->axes_ids
                                                              [0]].iunits, msg,
                                                    msg2, msg2);

                    first_time = tmp_vals[0];   /*store for later */
                }

                ierr = nc_put_vara_double(ncid, avar->time_nc_id, starts,
                                          counts, &tmp_vals[0]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_variadic(
                        "NetCDF error (%i: %s) writing time values for variable '%s' (%s)",
                        CMOR_CRITICAL,
                        ierr, nc_strerror(ierr), avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                }


/* -------------------------------------------------------------------- */
/*      ok now we need to store first and last stuff                    */
/* -------------------------------------------------------------------- */

                if (avar->ntimes_written == 0) {
/* -------------------------------------------------------------------- */
/*      ok first time we're putting data  in                            */
/* -------------------------------------------------------------------- */

                    avar->first_time = first_time;

                } else {

                    if (tmp_vals[0] < avar->last_time) {
                        cmor_handle_error_variadic(
                            "Time point: %lf ( %lf in output units) "
                            "is not monotonic last time was: %lf "
                            "(in output units), variable %s (table: %s)",
                            CMOR_CRITICAL,
                            time_vals[0], tmp_vals[0], avar->last_time,
                            avar->id,
                            cmor_tables[avar->ref_table_id].szTable_id);
                    }
                }

                avar->last_time = tmp_vals[ntimes_passed - 1];

                free(tmp_vals);
            } else {
/* -------------------------------------------------------------------- */
/*      checks if you need bounds or not                                */
/* -------------------------------------------------------------------- */

                if (cmor_tables[cmor_axes[avar->axes_ids[0]].ref_table_id].axes
                    [cmor_axes[avar->axes_ids[0]].ref_axis_id].
                    must_have_bounds == 1) {
                    cmor_handle_error_variadic(
                        "time axis must have bounds, please pass them to "
                        "cmor_write along with time values, variable %s, table %s",
                        CMOR_CRITICAL,
                        avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);

                }

                avar->first_bound = 1.e20;
                avar->last_bound = 1.e20;

                cmor_get_axis_attribute(avar->axes_ids[0], "units", 'c', &msg);
                cmor_get_cur_dataset_attribute("calendar", msg2);

                tmp_vals = malloc(ntimes_passed * sizeof(double));

                if (tmp_vals == NULL) {
                    cmor_handle_error_variadic(
                        "cannot malloc %i time vals for variable "
                        "'%s' (table: %s)",
                        CMOR_CRITICAL,
                        ntimes_passed, avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                }
                ierr = cmor_convert_time_values(time_vals, 'd', ntimes_passed,
                                                &tmp_vals[0],
                                                cmor_axes[avar->
                                                          axes_ids[0]].iunits,
                                                msg, msg2, msg2);

                ierr = nc_put_vara_double(ncid, avar->time_nc_id, starts,
                                          counts, tmp_vals);

                if (avar->ntimes_written == 0) {
/* -------------------------------------------------------------------- */
/*       ok first time we're putting data  in                           */
/* -------------------------------------------------------------------- */

                    avar->first_time = tmp_vals[0];
                }
                avar->last_time = tmp_vals[ntimes_passed - 1];

                free(tmp_vals);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_variadic(
                        "NetCDF error (%i: %s) writing times for variable '%s' "
                        "(table: %s), already written in file: %i",
                        CMOR_CRITICAL,
                        ierr, nc_strerror(ierr), avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id,
                        avar->ntimes_written);
                }
            }
        } else {
/* -------------------------------------------------------------------- */
/*      Ok we did not pass time values therefore it means they were     */
/*      defined via the axis                                            */
/* -------------------------------------------------------------------- */

            if (cmor_axes[avar->axes_ids[0]].values == NULL) {
                cmor_handle_error_variadic(
                    "variable '%s' (table: %s) you are passing %i "
                    "times but no values and you did not define "
                    "them via cmor_axis",
                    CMOR_CRITICAL,
                    avar->id,
                    cmor_tables[avar->ref_table_id].szTable_id,
                    ntimes_passed);
            }
            if (cmor_axes[avar->axes_ids[0]].bounds != NULL) {
/* -------------------------------------------------------------------- */
/*      ok at that stage the recentering must already be done so we     */
/*      just need to write the bounds                                   */
/* -------------------------------------------------------------------- */
                counts2[0] = counts[0];
                counts2[1] = 2;
                starts[1] = 0;
                ierr = nc_put_vara_double(ncid, avar->time_bnds_nc_id, starts,
                                          counts2,
                                          &cmor_axes[avar->
                                                     axes_ids[0]].bounds[starts
                                                                         [0] *
                                                                         2]);
                if (ierr != NC_NOERR) {
                    cmor_handle_error_variadic(
                        "NCError (%i: %s) writing time bounds values for "
                        "variable '%s' (table: %s)",
                        CMOR_CRITICAL,
                        ierr, nc_strerror(ierr), avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                }
/* -------------------------------------------------------------------- */
/*      ok we need to store first and last bounds                       */
/* -------------------------------------------------------------------- */
                if (avar->ntimes_written == 0) {
                    avar->first_bound =
                      cmor_axes[avar->axes_ids[0]].bounds[starts[0] * 2];
                }
                avar->last_bound =
                  cmor_axes[avar->axes_ids[0]].bounds[(starts[0]
                                                       + counts[0]) * 2 - 1];
            } else {
/* -------------------------------------------------------------------- */
/*      Checks wether you need bounds or not                            */
/* -------------------------------------------------------------------- */
                if (cmor_tables[cmor_axes[avar->axes_ids[0]].ref_table_id].axes
                    [cmor_axes[avar->axes_ids[0]].ref_axis_id].
                    must_have_bounds == 1) {
                    cmor_handle_error_variadic(
                        "time axis must have bounds, you defined it w/o "
                        "any for variable %s (table: %s)",
                        CMOR_CRITICAL,
                        avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                }
                avar->first_bound = 1.e20;
                avar->last_bound = 1.e20;
            }
            ierr = nc_put_vara_double(ncid, avar->time_nc_id, starts, counts,
                                      &cmor_axes[avar->
                                                 axes_ids[0]].values[starts
                                                                     [0]]);
            if (ierr != NC_NOERR) {
                cmor_handle_error_variadic(
                    "NCError (%i: %s) writing time values for variable '%s' (table: %s)",
                    CMOR_CRITICAL,
                    ierr, nc_strerror(ierr), avar->id,
                    cmor_tables[avar->ref_table_id].szTable_id);
            }
/* -------------------------------------------------------------------- */
/*      ok now we need to store first and last stuff                    */
/* -------------------------------------------------------------------- */

            if (avar->ntimes_written == 0) {
                avar->first_time =
                  cmor_axes[avar->axes_ids[0]].values[starts[0]];
            }

            avar->last_time = cmor_axes[avar->axes_ids[0]].values[starts[0]
                                                                  + counts[0] -
                                                                  1];
        }
    } else {
/* -------------------------------------------------------------------- */
/*      ok we did not pass time values therefore it means they were     */
/*      defined via the axis                                            */
/* -------------------------------------------------------------------- */
        ierr = -1;
/* -------------------------------------------------------------------- */
/*      look for time dimension                                         */
/* -------------------------------------------------------------------- */
        for (i = 0; i < avar->ndims; i++) {
            if (cmor_axes[avar->axes_ids[0]].axis == 'T') {
                ierr = i;
                break;
            }
        }

        if (ierr != -1) {

            if (cmor_axes[avar->axes_ids[ierr]].values == NULL) {
                cmor_handle_error_variadic(
                    "variable '%s' (table: %s) you are passing %i "
                    "times but no values and you did not define "
                    "them via cmor_axis",
                    CMOR_CRITICAL,
                    avar->id,
                    cmor_tables[avar->ref_table_id].szTable_id,
                    ntimes_passed);

            }

            avar->first_bound = 1.e20;
            avar->last_bound = 1.e20;

            if (cmor_axes[avar->axes_ids[ierr]].bounds != NULL) {
/* -------------------------------------------------------------------- */
/*      ok at that stage the recentering must already be done so we     */
/*      just need to write the bounds                                   */
/* -------------------------------------------------------------------- */

                counts2[0] = counts[0];
                counts2[1] = 2;
                starts[0] = 0;
                starts[1] = 0;
                ierr = nc_put_vara_double(ncid, avar->time_bnds_nc_id, starts,
                                          counts2,
                                          &cmor_axes[avar->
                                                     axes_ids[0]].bounds[starts
                                                                         [0] *
                                                                         2]);

                if (ierr != NC_NOERR) {
                    cmor_handle_error_variadic(
                        "NCError (%i: %s) writing time bounds values for "
                        "variable '%s' (table: %s)",
                        CMOR_CRITICAL,
                        ierr, nc_strerror(ierr), avar->id,
                        cmor_tables[avar->ref_table_id].szTable_id);
                }
                avar->first_bound = cmor_axes[avar->axes_ids[0]].bounds[0];
                avar->last_bound = cmor_axes[avar->axes_ids[0]].bounds[counts[0]
                                                                       * 2 - 1];
            }

            ierr = nc_put_vara_double(ncid, avar->time_nc_id, starts, counts,
                                      &cmor_axes[avar->
                                                 axes_ids[0]].values[starts
                                                                     [0]]);

            if (ierr != NC_NOERR) {

                cmor_handle_error_variadic(
                    "NCError (%i: %s) writing time values for "
                    "variable '%s' (table: %s)",
                    CMOR_CRITICAL,
                    ierr, nc_strerror(ierr), avar->id,
                    cmor_tables[avar->ref_table_id].szTable_id);

            }
/* -------------------------------------------------------------------- */
/*      ok now we need to store first and last stuff                    */
/* -------------------------------------------------------------------- */

            avar->first_time = cmor_axes[avar->axes_ids[0]].values[0];
            avar->last_time = cmor_axes[avar->axes_ids[0]].values[starts[0]
                                                                  + counts[0] -
                                                                  1];
        }
    }

    if (avar->isbounds) {
        counts[avar->ndims] = 2;
        starts[avar->ndims] = 0;
    }

    if (mtype == 'd') {
        ierr = nc_put_vara_double(ncid, avar->nc_var_id, starts, counts,
                                  data_tmp);
    } else if (mtype == 'f') {
        ierr = nc_put_vara_float(ncid, avar->nc_var_id, starts, counts,
                                 fdata_tmp);
    } else if (mtype == 'l') {
        ierr = nc_put_vara_long(ncid, avar->nc_var_id, starts, counts,
                                ldata_tmp);
    } else if (mtype == 'i') {
        ierr = nc_put_vara_int(ncid, avar->nc_var_id, starts, counts,
                               idata_tmp);
    }

    if (ierr != NC_NOERR) {
        cmor_handle_error_variadic(
            "NetCDF Error (%i: %s), writing variable '%s' (table %s) to file",
            CMOR_CRITICAL,
            ierr, nc_strerror(ierr), avar->id,
            cmor_tables[avar->ref_table_id].szTable_id);
    }

    avar->ntimes_written += ntimes_passed;

    if (mtype == 'd')
        free(data_tmp);
    else if (mtype == 'f')
        free(fdata_tmp);
    else if (mtype == 'l')
        free(ldata_tmp);
    else if (mtype == 'i')
        free(idata_tmp);

    cmor_pop_traceback();
    return (0);
}
