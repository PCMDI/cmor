#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "cmor.h"
#include "cmor_func_def.h"
#include <netcdf.h>
#include <udunits2.h>
#include <stdlib.h>
#include "cmor_locale.h"
#include <json-c/json.h>
#include <json-c/json_tokener.h>
#include <json-c/arraylist.h>
#include <sys/stat.h>

/************************************************************************/
/*                               wfgetc()                               */
/************************************************************************/
int wfgetc(FILE * afile)
{
    int i = fgetc(afile);

    while (i == '\r') {
        i = fgetc(afile);
    }
    return (i);
}

/************************************************************************/
/*                       cmor_get_table_attr                            */
/*                                                                      */
/*  tags for template used in input config file (.json)                 */
/*                                                                      */
/************************************************************************/
int cmor_get_table_attr(char *szToken, cmor_table_t * table, char *out)
{
    int i;
    t_symstruct lookuptable[] = {
        {"mip_era", table->mip_era},
        {"table", table->szTable_id},
        {"realm", table->realm},
        {"date", table->date},
        {"product", table->product},
        {"path", table->path},
        //           {"frequency",   table->frequency   },
        {"", ""},
        {"", ""},
        {"", ""}
    };

    int nKeys = (sizeof(lookuptable) / sizeof(t_symstruct));

    for (i = 0; i < nKeys; i++) {
        t_symstruct *sym = &lookuptable[i];
        if (strcmp(szToken, sym->key) == 0) {
            strcpy(out, sym->value);
            cmor_pop_traceback();
            return (0);
        }
    }

    cmor_pop_traceback();
    return (1);

}

/************************************************************************/
/*                          cmor_init_table()                           */
/************************************************************************/
void cmor_init_table(cmor_table_t * table, int id)
{
    int i;

    cmor_add_traceback("cmor_init_table");
    cmor_is_setup();
    /* init the table */
    table->id = id;
    table->nvars = -1;
    table->nformula = -1;
    table->naxes = -1;
    table->nexps = -1;
    table->nmappings = -1;
    table->cf_version = 1.7;
    table->cmor_version = 3.0;
    table->mip_era[0] = '\0';
    table->szTable_id[0] = '\0';
    table->realm[0] = '\0';
    table->date[0] = '\0';
    table->missing_value = 1.0e+20;
    table->int_missing_value = 2147483647;
    table->interval = 0.;
    table->interval_warning = .1;
    table->interval_error = .2;
    table->URL[0] = '\0';
    strcpy(table->product, "model_output");
    table->path[0] = '\0';
//    table->frequency[0] = '\0';
    table->nforcings = 0;
    for (i = 0; i < CMOR_MAX_ELEMENTS; i++) {
        table->expt_ids[i][0] = '\0';
        table->sht_expt_ids[i][0] = '\0';
        table->generic_levels[i][0] = '\0';
    }
    table->CV = NULL;

    cmor_pop_traceback();

}

/************************************************************************/
/*                    cmor_set_formula_entry()                          */
/************************************************************************/
int cmor_set_formula_entry(cmor_table_t * table,
                           char *formula_entry, json_object * json)
{
    extern int cmor_ntables;
    char szValue[CMOR_MAX_STRING];
    int nFormulaId;
    char *szTableId;
    cmor_var_def_t *formula;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    szTableId = cmor_table->szTable_id;

    cmor_add_traceback("cmor_set_formula_entry");
    cmor_is_setup();

    /* -------------------------------------------------------------------- */
    /*      Check number of formula                                         */
    /* -------------------------------------------------------------------- */
    cmor_table->nformula++;
    nFormulaId = cmor_table->nformula;
    formula = &cmor_table->formula[nFormulaId];

    if (nFormulaId >= CMOR_MAX_FORMULA) {
        cmor_handle_error_variadic(
            "Too many formula defined for table: %s",
            CMOR_CRITICAL,
            szTableId);
        cmor_ntables--;
        cmor_pop_traceback();
        return (1);
    }

    cmor_init_var_def(formula, cmor_ntables);
    cmor_set_var_def_att(formula, "id", formula_entry);

    json_object_object_foreach(json, attr, value) {
/* -------------------------------------------------------------------- */
/*  Attribute keys starting with "#" are seen as comments or examples   */
/*  and they are skipped!                                               */
/* -------------------------------------------------------------------- */

        if (attr[0] == '#')
            continue;

        strcpy(szValue, json_object_get_string(value));
        cmor_set_var_def_att(formula, attr, szValue);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_set_variable_entry()                         */
/************************************************************************/
int cmor_set_variable_entry(cmor_table_t * table,
                            char *variable_entry, json_object * json)
{
    extern int cmor_ntables;
    char szValue[CMOR_MAX_STRING];
    int nVarId;
    char *szTableId;
    array_list *jsonArray;
    json_object *jsonItem;
    size_t k, arrayLen;
    cmor_var_def_t *variable;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    szTableId = cmor_table->szTable_id;

    cmor_add_traceback("cmor_set_variable_entry");
    cmor_is_setup();

    /* -------------------------------------------------------------------- */
    /*      Check number of variables                                       */
    /* -------------------------------------------------------------------- */
    cmor_table->nvars++;
    nVarId = cmor_table->nvars;
    variable = &cmor_table->vars[nVarId];

    if (nVarId >= CMOR_MAX_ELEMENTS) {
        cmor_handle_error_variadic(
            "Too many variables defined for table: %s",
            CMOR_CRITICAL,
            szTableId);
        cmor_ntables--;
        cmor_pop_traceback();
        return (1);
    }

    cmor_init_var_def(variable, cmor_ntables);
    cmor_set_var_def_att(variable, "id", variable_entry);

    json_object_object_foreach(json, attr, value) {
/* -------------------------------------------------------------------- */
/*  Attribute keys starting with "#" are seen as comments or examples   */
/*  and they are skipped!                                               */
/* -------------------------------------------------------------------- */

        if (attr[0] == '#')
            continue;

/* -------------------------------------------------------------------- */
/*  Attribute values that are arrays will have their array elements     */
/*  combined into space-separated lists.                                */
/* -------------------------------------------------------------------- */
        if(json_object_is_type(value, json_type_array)) {
            jsonArray = json_object_get_array(value);
            arrayLen = array_list_length(jsonArray);
            for (k = 0; k < arrayLen; k++) {
                jsonItem = (json_object *) array_list_get_idx(jsonArray, k);
                if (k == 0) {
                    strcpy(szValue, json_object_get_string(jsonItem));
                } else {
                    strcat(szValue, " ");
                    strcat(szValue, json_object_get_string(jsonItem));
                }
            }
        } else {
            strcpy(szValue, json_object_get_string(value));
        }

        cmor_set_var_def_att(variable, attr, szValue);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                        cmor_set_axis_entry()                         */
/************************************************************************/
int cmor_set_axis_entry(cmor_table_t * table,
                        char *axis_entry, json_object * json)
{
    extern int cmor_ntables;
    char szValue[CMOR_MAX_STRING * 20];
    int nAxisId;
    char *szTableId;
    cmor_axis_def_t *axis;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    szTableId = cmor_table->szTable_id;

    cmor_add_traceback("cmor_set_axis_entry");
    cmor_is_setup();

    /* -------------------------------------------------------------------- */
    /*      Check number of axes                                            */
    /* -------------------------------------------------------------------- */
    cmor_table->naxes++;
    nAxisId = cmor_table->naxes;
    axis = &cmor_table->axes[nAxisId];

    if (nAxisId >= CMOR_MAX_ELEMENTS) {
        cmor_handle_error_variadic(
            "Too many axes defined for table: %s",
            CMOR_CRITICAL,
            szTableId);
        cmor_ntables--;
        cmor_pop_traceback();
        return (1);
    }
    axis = &cmor_table->axes[nAxisId];

    /* -------------------------------------------------------------------- */
    /*      Define Axis                                                     */
    /* -------------------------------------------------------------------- */
    cmor_init_axis_def(axis, cmor_ntables);
    cmor_set_axis_def_att(axis, "id", axis_entry);

    /* -------------------------------------------------------------------- */
    /*      Add axis value                                                  */
    /* -------------------------------------------------------------------- */
    json_object_object_foreach(json, attr, value) {
        if (attr[0] == '#') {
            continue;
        }
        strcpy(szValue, json_object_get_string(value));
        cmor_set_axis_def_att(axis, attr, szValue);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                        cmor_set_experiments()                        */
/************************************************************************/
int cmor_set_experiments(cmor_table_t * table, char att[CMOR_MAX_STRING],
                         char val[CMOR_MAX_STRING])
{
    extern int cmor_ntables;

    cmor_add_traceback("cmor_set_experiments");
    cmor_is_setup();
    table->nexps++;
    /* -------------------------------------------------------------------- */
    /*      Check number of experiments                                     */
    /* -------------------------------------------------------------------- */
    if (table->nexps > CMOR_MAX_ELEMENTS) {
        cmor_handle_error_variadic(
            "Table %s: Too many experiments defined",
            CMOR_CRITICAL,
            table->szTable_id);
        cmor_ntables--;
        cmor_pop_traceback();
        return (1);
    }

    /* -------------------------------------------------------------------- */
    /*      Insert experiment to table                                      */
    /* -------------------------------------------------------------------- */

    strncpy(table->sht_expt_ids[table->nexps], att, CMOR_MAX_STRING);
    strncpy(table->expt_ids[table->nexps], val, CMOR_MAX_STRING);

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                        cmor_set_dataset_att()                        */
/************************************************************************/
int cmor_set_dataset_att(cmor_table_t * table, char att[CMOR_MAX_STRING],
                         char val[CMOR_MAX_STRING])
{
    int n, i, j;
    float d, d2;
    char value[CMOR_MAX_STRING];
    char value2[CMOR_MAX_STRING];
    extern int cmor_ntables;

    cmor_add_traceback("cmor_set_dataset_att");
    cmor_is_setup();

    strncpy(value, val, CMOR_MAX_STRING);
    if(value[0] == '#') {
        return(0);
    }
/* -------------------------------------------------------------------- */
/*      Read non-block metadata.                                        */
/* -------------------------------------------------------------------- */
    if (strcmp(att, TABLE_HEADER_VERSION) == 0) {
        d2 = CMOR_VERSION_MAJOR;
        d = CMOR_VERSION_MINOR;
        while (d > 1.)
            d /= 10.;
        d2 += d;
        sscanf(value, "%f", &d);
        if (d > d2) {
            cmor_handle_error_variadic(
                "Table %s is defined for cmor_version %f, "
                "this library version is: %i.%i.%i, %f",
                CMOR_CRITICAL,
                table->szTable_id, d,
                CMOR_VERSION_MAJOR, CMOR_VERSION_MINOR,
                CMOR_VERSION_PATCH, d2);
            cmor_ntables--;
            cmor_pop_traceback();
            return (1);
        }
        table->cmor_version = d;

    } else if (strcmp(att, TABLE_HEADER_GENERIC_LEVS) == 0) {
        n = 0;
        i = 0;
        while (i < (strlen(value))) {
            while (value[i] == ' ')
                i++;
            j = 0;
            while (i < (strlen(value)) && value[i] != ' ') {
                table->generic_levels[n][j] = value[i];
                j++;
                i++;
            }
            table->generic_levels[n][j] = '\0';
            n += 1;
        }

    } else if (strcmp(att, TABLE_HEADER_CONVENTIONS) == 0) {
        strncpy(table->Conventions, val, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_DATASPECSVERSION) == 0) {
        strncpy(table->data_specs_version, val, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_MIP_ERA) == 0) {
        strncpy(table->mip_era, value, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_REALM) == 0) {
        strncpy(table->realm, value, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_TABLE_DATE) == 0) {
        strncpy(table->date, value, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_BASEURL) == 0) {
        strncpy(table->URL, value, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_FORCINGS) == 0) {
        cmor_convert_string_to_list(value,
                                    'c',
                                    (void **)&table->forcings,
                                    &table->nforcings);

    } else if (strcmp(att, TABLE_HEADER_PRODUCT) == 0) {
        strncpy(table->product, value, CMOR_MAX_STRING);

//      } else if (strcmp(att, TABLE_HEADER_FREQUENCY) == 0) {
//              strncpy(table->frequency, value, CMOR_MAX_STRING);

    } else if (strcmp(att, TABLE_HEADER_TABLE_ID) == 0) {
        for (n = 0; n == cmor_ntables; n++) {
            if (strcmp(cmor_tables[n].szTable_id, value) == 0) {
                cmor_handle_error_variadic(
                    "Table %s is already defined",
                    CMOR_CRITICAL,
                    table->szTable_id);
                cmor_ntables--;
                cmor_pop_traceback();
                return (1);
            }
        }
        n = strlen(value);
        for (i = n - 1; i > 0; i--) {
            if (value[i] == ' ')
                break;
        }
        if (value[i] == ' ')
            i++;

        for (j = i; j < n; j++)
            value2[j - i] = value[j];
        value2[n - i] = '\0';
        strcpy(table->szTable_id, value2);

/* -------------------------------------------------------------------- */
/*      Save all experiment id                                          */
/* -------------------------------------------------------------------- */
    } else if (strcmp(att, TABLE_EXPIDS) == 0) {
        table->nexps++;
        if (table->nexps > CMOR_MAX_ELEMENTS) {
            cmor_handle_error_variadic(
                "Table %s: Too many experiments defined",
                CMOR_CRITICAL,
                table->szTable_id);
            cmor_ntables--;
            cmor_pop_traceback();
            return (1);
        }

        if (value[0] == '\'')
            for (n = 0; n < strlen(value) - 1; n++)
                value[n] = value[n + 1];        /* removes leading "'" */
        n = strlen(value);

        if (value[n - 2] == '\'')
            value[n - 2] = '\0';        /*removes trailing "'" */
/* -------------------------------------------------------------------- */
/*      ok here we look for a ' which means there is                    */
/*      a short name associated with it                                 */
/* -------------------------------------------------------------------- */
        n = -1;
        for (j = 0; j < strlen(value); j++) {
            if (value[j] == '\'') {
                n = j;
                break;
            }
        }
        if (n == -1) {
            strncpy(table->expt_ids[table->nexps], value, CMOR_MAX_STRING);
            strcpy(table->sht_expt_ids[table->nexps], "");
        } else {
/* -------------------------------------------------------------------- */
/*      ok looks like we have a short name let clook for the next '     */
/* -------------------------------------------------------------------- */
            i = -1;
            for (j = n + 1; j < strlen(value); j++) {
                if (value[j] == '\'')
                    i = j;
            }
/* -------------------------------------------------------------------- */
/*      ok we must have a ' in our exp_id_ok                            */
/* -------------------------------------------------------------------- */
            if (i == -1) {
                strncpy(table->expt_ids[table->nexps], value, CMOR_MAX_STRING);
                strcpy(table->sht_expt_ids[table->nexps], "");
            } else {
                for (j = i + 1; j < strlen(value); j++) {
                    value2[j - i - 1] = value[j];
                    value2[j - i] = '\0';
                }
                strncpy(table->sht_expt_ids[table->nexps], value2,
                        CMOR_MAX_STRING);
                value[n] = '\0';
                strncpy(table->expt_ids[table->nexps], value, CMOR_MAX_STRING);
            }
        }
    } else if (strcmp(att, TABLE_HEADER_APRX_INTRVL) == 0) {
        sscanf(value, "%lf", &table->interval);
    } else if (strcmp(att, TABLE_HEADER_APRX_INTRVL_ERR) == 0) {
        sscanf(value, "%lf", &table->interval_error);
    } else if (strcmp(att, TABLE_HEADER_APRX_INTRVL_WRN) == 0) {
        sscanf(value, "%lf", &table->interval_warning);
    } else if (strcmp(att, TABLE_HEADER_MISSING_VALUE) == 0) {
        sscanf(value, "%lf", &table->missing_value);
    } else if (strcmp(att, TABLE_HEADER_INT_MISSING_VALUE) == 0) {
        sscanf(value, "%ld", &table->int_missing_value);
    } else if (strcmp(att, TABLE_HEADER_MAGIC_NUMBER) == 0) {

    } else {

        cmor_handle_error_variadic(
            "table: %s, This keyword: %s value (%s) "
            "is not a valid table header entry.!\n "
            "Use the user input JSON file to add custom attributes.",
            CMOR_WARNING,
            table->szTable_id, att, value);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                           cmor_set_table()                           */
/************************************************************************/

int cmor_set_table(int table)
{
    extern int CMOR_TABLE;

    cmor_add_traceback("cmor_set_table");
    cmor_is_setup();
    if (table > cmor_ntables) {
        cmor_handle_error_variadic("Invalid table number: %i", CMOR_CRITICAL, table);
    }
    if (cmor_tables[table].szTable_id[0] == '\0') {
        cmor_handle_error_variadic(
            "Invalid table: %i , not loaded yet!",
            CMOR_CRITICAL,
            table);
    }
    CMOR_TABLE = table;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_load_table()                              */
/************************************************************************/
int cmor_load_table(char szTable[CMOR_MAX_STRING], int *table_id)
{
    int rc;
    char *szPath;
    char *szTableName;
    char szControlFilenameJSON[CMOR_MAX_STRING];
    char szAxisEntryFilenameJSON[CMOR_MAX_STRING];
    char szFormulaVarFilenameJSON[CMOR_MAX_STRING];
    char szCV[CMOR_MAX_STRING];
    char szAxisEntryFN[CMOR_MAX_STRING];
    char szFormulaVarFN[CMOR_MAX_STRING];
    struct stat st;
    cmor_add_traceback("cmor_load_table");

    if (cmor_ntables == (CMOR_MAX_TABLES - 1)) {
        cmor_pop_traceback();
        cmor_handle_error_variadic(
            "You cannot load more than %d tables",
            CMOR_CRITICAL,
            CMOR_MAX_TABLES);
        return (-1);
    }

    rc = cmor_get_cur_dataset_attribute(GLOBAL_CV_FILENAME, szCV);
    rc = cmor_get_cur_dataset_attribute(CMOR_AXIS_ENTRY_FILE, szAxisEntryFN);
    rc = cmor_get_cur_dataset_attribute(CMOR_FORMULA_VAR_FILE, szFormulaVarFN);

/* -------------------------------------------------------------------- */
/*  build string "path/<CV>.json"                                */
/* -------------------------------------------------------------------- */
    szTableName = strdup(szTable);
    szPath = dirname(szTableName);
/* -------------------------------------------------------------------- */
/*  build string "path/filename.json"                                   */
/* -------------------------------------------------------------------- */
    strcpy(szControlFilenameJSON, szPath);
    strcat(szControlFilenameJSON, "/");
    strcat(szControlFilenameJSON, szCV);
    strcpy(szAxisEntryFilenameJSON, szPath);
    strcat(szAxisEntryFilenameJSON, "/");
    strcat(szAxisEntryFilenameJSON, szAxisEntryFN);
    strcpy(szFormulaVarFilenameJSON, szPath);
    strcat(szFormulaVarFilenameJSON, "/");
    strcat(szFormulaVarFilenameJSON, szFormulaVarFN);

/* -------------------------------------------------------------------- */
/*  try to load table from directory where table is found or from the   */
/*  cmor_input_path                                                     */
/* -------------------------------------------------------------------- */
    rc = stat(szControlFilenameJSON, &st);
    if (rc != 0) {
        strcpy(szControlFilenameJSON, cmor_input_path);
        strcat(szControlFilenameJSON, "/");
        strcat(szControlFilenameJSON, szCV);
        strcpy(szAxisEntryFilenameJSON, szPath);
        strcat(szAxisEntryFilenameJSON, "/");
        strcat(szAxisEntryFilenameJSON, szAxisEntryFN);
        strcpy(szFormulaVarFilenameJSON, szPath);
        strcat(szFormulaVarFilenameJSON, "/");
        strcat(szFormulaVarFilenameJSON, szFormulaVarFN);

    }
    /* -------------------------------------------------------------------- */
    /*      Is the table already in memory?                                 */
    /* -------------------------------------------------------------------- */
    rc = cmor_search_table(szTable, table_id);

    if (rc == TABLE_FOUND) {
        cmor_setDefaultGblAttr(*table_id);
        return (TABLE_SUCCESS);
    }

    if (rc == TABLE_NOTFOUND) {
        cmor_ntables += 1;
        cmor_init_table(&cmor_tables[cmor_ntables], cmor_ntables);
        *table_id = cmor_ntables;

        strcpy(cmor_tables[cmor_ntables].path, szTable);
        cmor_set_cur_dataset_attribute_internal(CV_INPUTFILENAME,
                                                szControlFilenameJSON, 1);
        rc = cmor_load_table_internal(szAxisEntryFilenameJSON, table_id);
        if (rc != TABLE_SUCCESS) {
            cmor_handle_error_variadic(
                "Can't open/read JSON table %s", 
                CMOR_CRITICAL,
                szAxisEntryFilenameJSON);
            return (1);
        }
        rc = cmor_load_table_internal(szTable, table_id);
        if (rc != TABLE_SUCCESS) {
            cmor_handle_error_variadic(
                "Can't open/read JSON table %s",
                CMOR_CRITICAL,
                szTable);
            return (1);
        }
        rc = cmor_load_table_internal(szFormulaVarFilenameJSON, table_id);
        if (rc != TABLE_SUCCESS) {
            cmor_handle_error_variadic(
                "Can't open/read JSON table %s",
                CMOR_CRITICAL,
                szFormulaVarFilenameJSON);
            return (1);
        }
        rc = cmor_load_table_internal(szControlFilenameJSON, table_id);
        if (rc != TABLE_SUCCESS) {
            cmor_handle_error_variadic(
                "Can't open/read JSON table %s",
                CMOR_CRITICAL,
                szControlFilenameJSON);
            return (1);
        }

    } else if (rc == TABLE_FOUND) {
        rc = TABLE_SUCCESS;
    }

    cmor_setDefaultGblAttr(*table_id);

    free(szTableName);

    return (rc);
}

/************************************************************************/
/*                       cmor_search_table()                            */
/************************************************************************/
int cmor_search_table(char szTable[CMOR_MAX_STRING], int *table_id)
{
    int i;
    for (i = 0; i < cmor_ntables + 1; i++) {

        if (strcmp(cmor_tables[i].path, szTable) == 0) {
            CMOR_TABLE = i;
            *table_id = i;
            cmor_pop_traceback();
            return (TABLE_FOUND);
        }
    }
    cmor_pop_traceback();
    return (TABLE_NOTFOUND);
}

/************************************************************************/
/*                   cmor_load_table_internal()                         */
/************************************************************************/
int cmor_load_table_internal(char szTable[CMOR_MAX_STRING], int *table_id)
{
    FILE *table_file;
    char word[CMOR_MAX_STRING];
    int n;
    int done = 0;
    extern int CMOR_TABLE, cmor_ntables;
    extern char cmor_input_path[CMOR_MAX_STRING];
    char szVal[1024000];
    char *buffer = NULL;
    int nTableSize, read_size;
    json_object *json_obj;

    cmor_add_traceback("cmor_load_table_internal");
    cmor_is_setup();

    table_file = fopen(szTable, "r");
    if (table_file == NULL) {
        if (szTable[0] != '/') {
            snprintf(word, CMOR_MAX_STRING, "%s/%s", cmor_input_path, szTable);
            table_file = fopen(word, "r");
        }
        if (table_file == NULL) {
            snprintf(word, CMOR_MAX_STRING, "%s/share/%s", CMOR_PREFIX,
                     szTable);
            table_file = fopen(word, "r");
        }
        if (table_file == NULL) {
            cmor_handle_error_variadic(
                "Could not find file: %s",
                CMOR_NORMAL,
                szTable);
            cmor_ntables -= 1;
            cmor_pop_traceback();
            return (TABLE_ERROR);
        }
    }

/* -------------------------------------------------------------------- */
/*      ok now we need to store the md5                                 */
/* -------------------------------------------------------------------- */
    cmor_md5(table_file, cmor_tables[cmor_ntables].md5);

/* -------------------------------------------------------------------- */
/*      Read the entire table in memory                                 */
/* -------------------------------------------------------------------- */
    fseek(table_file, 0, SEEK_END);
    nTableSize = ftell(table_file);
    rewind(table_file);
    buffer = (char *)malloc(sizeof(char) * (nTableSize + 1));
    read_size = fread(buffer, sizeof(char), nTableSize, table_file);
    buffer[nTableSize] = '\0';

/* -------------------------------------------------------------------- */
/*      print errror and exist if not a JSON file                       */
/* -------------------------------------------------------------------- */

    if (buffer[0] != '{') {
        free(buffer);
        buffer = NULL;
        cmor_handle_error_variadic(
            "Could not understand file \"%s\" Is this a JSON CMOR table?",
            CMOR_CRITICAL,
            szTable);
        cmor_ntables--;
        cmor_pop_traceback();
        return (TABLE_ERROR);
    }
/* -------------------------------------------------------------------- */
/*      print error and exit if file was not completely read            */
/* -------------------------------------------------------------------- */
    if (nTableSize != read_size) {
        free(buffer);
        buffer = NULL;
        cmor_handle_error_variadic(
            "Could not read file %s check file permission",
            CMOR_CRITICAL,
            word);
        cmor_ntables--;
        cmor_pop_traceback();
        return (TABLE_ERROR);
    }

/* -------------------------------------------------------------------- */
/*     parse buffer into json object                                    */
/* -------------------------------------------------------------------- */
    json_obj = json_tokener_parse(buffer);
    if (json_obj == NULL) {
        cmor_handle_error_variadic(
            "Please validate JSON File!\n"
            "USE: http://jsonlint.com/\n"
            "Syntax Error in table: %s\n " "%s", 
            CMOR_CRITICAL,
            szTable, buffer);
        cmor_pop_traceback();
        return (TABLE_ERROR);
    }

/* -------------------------------------------------------------------- */
/*     check for null values in JSON                                    */
/* -------------------------------------------------------------------- */
    if(cmor_validate_json(json_obj) != 0) {
        cmor_handle_error_variadic(
            "There are invalid null values in table: %s",
            CMOR_CRITICAL,
            szTable);
        cmor_pop_traceback();
        return (TABLE_ERROR);
    }

    json_object_object_foreach(json_obj, key, value) {

        if (key[0] == '#') {
            continue;
        }
        if (value == 0) {
            return (TABLE_ERROR);
        }
        strcpy(szVal, json_object_get_string(value));
/* -------------------------------------------------------------------- */
/*      Now let's see what we found                                     */
/* -------------------------------------------------------------------- */
        if (strcmp(key, JSON_KEY_HEADER) == 0) {
/* -------------------------------------------------------------------- */
/*      Fill up all global attributer found in header section           */
/* -------------------------------------------------------------------- */
            json_object_object_foreach(value, key, globalAttr) {
                if (key[0] == '#') {
                    continue;
                }
                if (globalAttr == NULL) {
                    return (TABLE_ERROR);
                }
                strcpy(szVal, json_object_get_string(globalAttr));
                if (cmor_set_dataset_att(&cmor_tables[cmor_ntables], key,
                                         szVal) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done = 1;

        } else if (strcmp(key, JSON_KEY_EXPERIMENT) == 0) {
            json_object_object_foreach(value, shortname, experiment) {
                if (shortname[0] == '#') {
                    continue;
                }
                if (experiment == NULL) {
                    return (TABLE_ERROR);
                }

                strcpy(szVal, json_object_get_string(experiment));
                if (cmor_set_experiments(&cmor_tables[cmor_ntables],
                                         shortname, szVal) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done = 1;
        } else if (strcmp(key, JSON_KEY_AXIS_ENTRY) == 0) {
            json_object_object_foreach(value, axisname, attributes) {

                if (axisname[0] == '#') {
                    continue;
                }
                if (attributes == NULL) {
                    return (TABLE_ERROR);
                }
                if (cmor_set_axis_entry(&cmor_tables[cmor_ntables],
                                        axisname, attributes) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done = 1;
        } else if (strcmp(key, JSON_KEY_FORMULA_ENTRY) == 0) {
            json_object_object_foreach(value, formulaname, attributes) {

                if (formulaname[0] == '#') {
                    continue;
                }
                if (attributes == NULL) {
                    return (TABLE_ERROR);
                }
                if (cmor_set_formula_entry(&cmor_tables[cmor_ntables],
                                           formulaname, attributes) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done = 1;
        } else if (strcmp(key, JSON_KEY_VARIABLE_ENTRY) == 0) {
            json_object_object_foreach(value, varname, attributes) {

                if (varname[0] == '#') {
                    continue;
                }
                if (attributes == NULL) {
                    return (TABLE_ERROR);
                }
                if (cmor_set_variable_entry(&cmor_tables[cmor_ntables],
                                            varname, attributes) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done = 1;
        } else if (strncmp(key, JSON_KEY_CV_ENTRY, 2) == 0) {

            if (cmor_CV_set_entry(&cmor_tables[cmor_ntables], value) == 1) {
                cmor_pop_traceback();
                return (TABLE_ERROR);
            }
            done = 1;

        } else if (strcmp(key, JSON_KEY_MAPPING_ENTRY) == 0) {
/* -------------------------------------------------------------------- */
/*      Work on mapping entries                                         */
/* -------------------------------------------------------------------- */
            cmor_tables[cmor_ntables].nmappings++;
            if (cmor_tables[cmor_ntables].nmappings >= CMOR_MAX_ELEMENTS) {
                cmor_handle_error_variadic(
                    "Too many mappings defined for table: %s",
                    CMOR_CRITICAL,
                    cmor_tables[cmor_ntables].szTable_id);
                cmor_ntables--;
                cmor_pop_traceback();
                return (TABLE_ERROR);
            }
            json_object_object_foreach(value, mapname, jsonValue) {

                if (mapname[0] == '#') {
                    continue;
                }
                if (mapname == NULL) {
                    return (TABLE_ERROR);
                }

                char szLastMapID[CMOR_MAX_STRING];
                char szCurrMapID[CMOR_MAX_STRING];
                cmor_table_t *psCurrCmorTable;

                psCurrCmorTable = &cmor_tables[cmor_ntables];

                int nMap;
                nMap = psCurrCmorTable->nmappings;

                for (n = 0; n < nMap - 1; n++) {

                    strcpy(szLastMapID, psCurrCmorTable->mappings[nMap].id);
                    strcpy(szCurrMapID, psCurrCmorTable->mappings[n].id);

                    if (strcmp(szLastMapID, szCurrMapID) == 0) {
                        cmor_handle_error_variadic(
                            "mapping: %s already defined within this table (%s)",
                            CMOR_CRITICAL,
                            cmor_tables[cmor_ntables].mappings[n].id,
                            cmor_tables[cmor_ntables].szTable_id);
                    };
                }
/* -------------------------------------------------------------------- */
/*      init the variable def                                           */
/* -------------------------------------------------------------------- */
                cmor_init_grid_mapping(&psCurrCmorTable->mappings[nMap],
                                       mapname);
                json_object_object_foreach(jsonValue, key, mappar) {

                    if (key[0] == '#') {
                        continue;
                    }
                    if (mapname == NULL) {
                        return (TABLE_ERROR);
                    }

                    char param[CMOR_MAX_STRING];

                    strcpy(param, json_object_get_string(mappar));

                    cmor_set_mapping_attribute(&psCurrCmorTable->mappings
                                               [psCurrCmorTable->nmappings],
                                               key, param);

                }

            }
            done = 1;

        } else {
/* -------------------------------------------------------------------- */
/*      nothing known we will not be setting any attributes!            */
/* -------------------------------------------------------------------- */

            cmor_handle_error_variadic(
                "unknown section: %s, for table: %s",
                CMOR_WARNING,
                key, cmor_tables[cmor_ntables].szTable_id);
        }

/* -------------------------------------------------------------------- */
/*      First check for table/dataset mode values                       */
/* -------------------------------------------------------------------- */

        if (done == 1) {
            done = 0;
        } else {
            cmor_handle_error_variadic(
                "attribute for unknown section: %s,%s (table: %s)",
                CMOR_WARNING,
                key, szVal, cmor_tables[cmor_ntables].szTable_id);
            /*printf("attribute for unknown section\n"); */
        }
    }
    *table_id = cmor_ntables;
    CMOR_TABLE = cmor_ntables;
    if (table_file != NULL) {
        fclose(table_file);
        table_file = NULL;
    }
    cmor_pop_traceback();
    free(buffer);
    json_object_put(json_obj);
    return (TABLE_SUCCESS);
}


/************************************************************************/
/*                      cmor_validate_json()                            */
/************************************************************************/
int cmor_validate_json(json_object *json)
{
    json_object *value;
    array_list *array;
    size_t length, k;

    if (json_object_is_type(json, json_type_null)) {
        // null is invalid
        return 1;
    } else if (json_object_is_type(json, json_type_object)) {
        // validate values within JSON object
        json_object_object_foreach(json, key, value) {
            if (cmor_validate_json(value) == 1)
                return 1;
        }
    } else if (json_object_is_type(json, json_type_array)) {
        // validate values within JSON list
        array = json_object_get_array(json);
        length = array_list_length(array);
        for (k = 0; k < length; k++) {
            value = (json_object *) array_list_get_idx(array, k);
            if (cmor_validate_json(value) == 1)
                return 1;
        }
    }

    return 0;

}