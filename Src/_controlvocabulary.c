#include <Python.h>
#define NPY_NO_DEPRECATED_API  NPY_1_10_API_VERSION

#include "numpy/arrayobject.h"
#include "cmor.h"

extern int cmor_CV_variable(int *, char *, char *, float *, int *,
                            double, double, double, double);

/************************************************************************/
/*                       PyCV_checkFilename                             */
/************************************************************************/
static PyObject *PyCV_checkFilename(PyObject * self, PyObject * args)
{
    int ntable;
    int varid;
    char *szInTimeCalendar;
    char *szInTimeUnits;
    char *infile;
    int nTimeCalLen;
    int nTimeUnitsLen;
    int ninfile;
    int ierr;

    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "iis#s#s#", &ntable, &varid,
                          &szInTimeCalendar, &nTimeCalLen,
                          &szInTimeUnits, &nTimeUnitsLen, &infile, &ninfile)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkFilename(cmor_tables[ntable].CV, varid,
                          szInTimeCalendar, szInTimeUnits, infile);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                       PyCV_checkSubExpID                             */
/************************************************************************/
static PyObject *PyCV_checkSubExpID(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkSubExpID(cmor_tables[nVarRefTblID].CV);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                     PyCV_checkParentExpID                            */
/************************************************************************/
static PyObject *PyCV_checkParentExpID(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkParentExpID(cmor_tables[nVarRefTblID].CV);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                     PyCV_setInstitution()                            */
/************************************************************************/
static PyObject *PyCV_setInstitution(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_setInstitution(cmor_tables[nVarRefTblID].CV);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                       PyCV_checkSourceID()                           */
/************************************************************************/
static PyObject *PyCV_checkSourceID(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }
    ierr = cmor_CV_checkSourceID(cmor_tables[nVarRefTblID].CV);
    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                   PyCV_checkExperiment()                             */
/************************************************************************/
static PyObject *PyCV_checkExperiment(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkExperiment(cmor_tables[nVarRefTblID].CV);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                     PyCV_checkGrids()                                */
/************************************************************************/
static PyObject *PyCV_checkGrids(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkGrids(cmor_tables[nVarRefTblID].CV);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                   PyCV_checkFurtherInfoURL()                         */
/************************************************************************/
static PyObject *PyCV_checkFurtherInfoURL(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;

    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkFurtherInfoURL(nVarRefTblID);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                      PyCV_GblAttributes()                            */
/************************************************************************/
static PyObject *PyCV_GblAttributes(PyObject * self, PyObject * args)
{
    int nVarRefTblID;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &nVarRefTblID)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_CV_checkGblAttributes(cmor_tables[nVarRefTblID].CV);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                       PyCV_checkISOTime()                            */
/************************************************************************/
static PyObject *PyCV_checkISOTime(PyObject * self, PyObject * args)
{
    int ierr;
    cmor_is_setup();

    ierr = cmor_CV_checkISOTime(GLOBAL_ATT_CREATION_DATE);

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                  PyCMOR_set_cur_dataset_attribute()                  */
/************************************************************************/
static PyObject *PyCMOR_set_cur_dataset_attribute(PyObject * self,
                                                  PyObject * args)
{
    char *name;
    char *value;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "ss", &name, &value))
        return (NULL);

    ierr = cmor_set_cur_dataset_attribute(name, value, 1);

    if (ierr != 0)
        return NULL;

    Py_INCREF(Py_None);

    return (Py_None);
}

/************************************************************************/
/*                  PyCMOR_get_cur_dataset_attribute()                  */
/************************************************************************/
static PyObject *PyCMOR_get_cur_dataset_attribute(PyObject * self,
                                                  PyObject * args)
{
    char *name;
    char value[CMOR_MAX_STRING];
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    ierr = cmor_get_cur_dataset_attribute(name, value);
    if (ierr != 0)
        return NULL;
    return (Py_BuildValue("s", value));
}

/************************************************************************/
/*                  PyCMOR_has_cur_dataset_attribute()                  */
/************************************************************************/
static PyObject *PyCMOR_has_cur_dataset_attribute(PyObject * self,
                                                  PyObject * args)
{
    char *name;
    int ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    ierr = cmor_has_cur_dataset_attribute(name);
    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                   PyCMOR_set_variable_attribute()                    */
/************************************************************************/
static PyObject *PyCMOR_set_variable_attribute(PyObject * self, PyObject * args)
{
    char *name;
    char *value;
    int ierr, var_id;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "iss", &var_id, &name, &value))
        return NULL;
    ierr = cmor_set_variable_attribute(var_id, name, 'c', (void *)value);
    if (ierr != 0)
        return NULL;
    Py_INCREF(Py_None);
    return (Py_None);
}
/************************************************************************/
/*                         PyCV_reset_Error()                           */
/************************************************************************/
static PyObject *PyCV_reset_Error(PyObject * self, PyObject * args)
{
    cmor_is_setup();
    CV_ERROR = 0;
    return (Py_None);
}

/************************************************************************/
/*                         PyCV_set_Error()                             */
/************************************************************************/
static PyObject *PyCV_set_Error(PyObject * self, PyObject * args)
{
    cmor_is_setup();
    CV_ERROR = 1;
    return (Py_None);
}

/************************************************************************/
/*                         PyCV_get_Error()                             */
/************************************************************************/
static PyObject *PyCV_get_Error(PyObject * self, PyObject * args)
{
    int cv_error;
    cmor_is_setup();

    cv_error = get_CV_Error();
    return (Py_BuildValue("i", cv_error));
}

/************************************************************************/
/*                 PyCMOR_get_variable_attribute_list()                 */
/************************************************************************/
static PyObject *PyCMOR_get_variable_attribute_list(PyObject * self,
                                                    PyObject * args)
{

    int index;
    int var_id;
    cmor_is_setup();
    char attribute_name[CMOR_MAX_STRING];
    char type;
    int i;

    if (!PyArg_ParseTuple(args, "i", &var_id)) {
        return NULL;
    }
    index = cmor_vars[var_id].nattributes;
    if (index == -1)
        return NULL;

    PyObject *dico = PyDict_New();
    for (i = 0; i < index; i++) {
        strcpy(attribute_name, cmor_vars[var_id].attributes[i]);
        if (attribute_name[0] == '\0')
            continue;
        type = cmor_vars[var_id].attributes_type[i];
        if (type == 'c') {
            PyDict_SetItemString(dico,
                                 cmor_vars[var_id].attributes[i],
                                 Py_BuildValue("s",
                                               cmor_vars
                                               [var_id].attributes_values_char
                                               [i]));
        } else if (type == 'f') {
            PyDict_SetItemString(dico,
                                 cmor_vars[var_id].attributes[i],
                                 Py_BuildValue("f",
                                               (float)
                                               cmor_vars
                                               [var_id].attributes_values_num
                                               [i]));
        } else if (type == 'i') {
            PyDict_SetItemString(dico,
                                 cmor_vars[var_id].attributes[i],
                                 Py_BuildValue("i",
                                               (int)
                                               cmor_vars
                                               [var_id].attributes_values_num
                                               [i]));
        } else if (type == 'l') {
            PyDict_SetItemString(dico,
                                 cmor_vars[var_id].attributes[i],
                                 Py_BuildValue("l",
                                               (long)
                                               cmor_vars
                                               [var_id].attributes_values_num
                                               [i]));
        } else {
            PyDict_SetItemString(dico,
                                 cmor_vars[var_id].attributes[i],
                                 Py_BuildValue("d",
                                               (double)
                                               cmor_vars
                                               [var_id].attributes_values_num
                                               [i]));
        }
    }
    cmor_pop_traceback();
    return (dico);
}

/************************************************************************/
/*                   PyCMOR_get_variable_attribute()                    */
/************************************************************************/
static PyObject *PyCMOR_get_variable_attribute(PyObject * self, PyObject * args)
{
    char *name;
    char value[CMOR_MAX_STRING];
    float fValue;
    int ierr, var_id;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "is", &var_id, &name))
        return NULL;
    ierr = cmor_get_variable_attribute(var_id, name, (void *)value);
    if (ierr != 0)
        return NULL;
    if ((strcmp(name, VARIABLE_ATT_FILLVAL) == 0) ||
        (strcmp(name, VARIABLE_ATT_MISSINGVALUES) == 0)) {
        ierr = cmor_get_variable_attribute(var_id, name, &fValue);
        return (Py_BuildValue("f", fValue));
    }
    return (Py_BuildValue("s", value));
}

/************************************************************************/
/*                   PyCMOR_has_variable_attribute()                    */
/************************************************************************/
static PyObject *PyCMOR_has_variable_attribute(PyObject * self, PyObject * args)
{
    char *name;
    int ierr, var_id;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "is", &var_id, &name))
        return NULL;
    ierr = cmor_has_variable_attribute(var_id, name);
    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                            PyCMOR_setup()                            */
/************************************************************************/
static PyObject *PyCMOR_setup(PyObject * self, PyObject * args)
{
    int mode, ierr, netcdf, verbosity, createsub;
    char *path;
    char *logfile;

    if (!PyArg_ParseTuple
        (args, "siiisi", &path, &netcdf, &verbosity, &mode, &logfile,
         &createsub))
        return NULL;
    if (strcmp(logfile, "") == 0) {
        ierr = cmor_setup(path, &netcdf, &verbosity, &mode, NULL, &createsub);
    } else {
        ierr =
          cmor_setup(path, &netcdf, &verbosity, &mode, logfile, &createsub);
    }
    strncpytrim(cmor_current_dataset.path_template,
                CMOR_DEFAULT_PATH_TEMPLATE, CMOR_MAX_STRING);

    strncpytrim(cmor_current_dataset.file_template,
                CMOR_DEFAULT_FILE_TEMPLATE, CMOR_MAX_STRING);

    strncpytrim(cmor_current_dataset.furtherinfourl,
                CMOR_DEFAULT_FURTHERURL_TEMPLATE, CMOR_MAX_STRING);

    if (ierr != 0)
        return NULL;
    Py_INCREF(Py_None);
    return (Py_None);

}

/************************************************************************/
/*                         PyCMOR_load_table()                          */
/************************************************************************/

static PyObject *PyCMOR_load_table(PyObject * self, PyObject * args)
{
    int ierr, table_id;
    char *table;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "s", &table))
        return NULL;
    ierr = cmor_load_table(table, &table_id);
    if (ierr != 0) {
        return NULL;
    }
    return (Py_BuildValue("i", table_id));
}

/************************************************************************/
/*                        PyCMOR_getincvalues()                         */
/************************************************************************/

static PyObject *PyCMOR_getincvalues(PyObject * self, PyObject * args)
{
    char *att_name;

    if (!PyArg_ParseTuple(args, "s", &att_name)) {
        return NULL;
    }
    if (strcmp(att_name, "CMOR_MAX_STRING") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_STRING));
    } else if (strcmp(att_name, "CMOR_MAX_ELEMENTS") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_ELEMENTS));
    } else if (strcmp(att_name, "CMOR_MAX_AXES") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_AXES));
    } else if (strcmp(att_name, "CMOR_MAX_VARIABLES") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_VARIABLES));
    } else if (strcmp(att_name, "CMOR_MAX_GRIDS") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_GRIDS));
    } else if (strcmp(att_name, "CMOR_MAX_DIMENSIONS") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_DIMENSIONS));
    } else if (strcmp(att_name, "CMOR_MAX_ATTRIBUTES") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_ATTRIBUTES));
    } else if (strcmp(att_name, "CMOR_MAX_ERRORS") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_ERRORS));
    } else if (strcmp(att_name, "CMOR_MAX_TABLES") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_TABLES));
    } else if (strcmp(att_name, "CMOR_MAX_GRID_ATTRIBUTES") == 0) {
        return (Py_BuildValue("i", CMOR_MAX_GRID_ATTRIBUTES));
    } else if (strcmp(att_name, "CMOR_QUIET") == 0) {
        return (Py_BuildValue("i", CMOR_QUIET));
    } else if (strcmp(att_name, "CMOR_EXIT_ON_MAJOR") == 0) {
        return (Py_BuildValue("i", CMOR_EXIT_ON_MAJOR));
    } else if (strcmp(att_name, "CMOR_EXIT") == 0) {
        return (Py_BuildValue("i", CMOR_EXIT));
    } else if (strcmp(att_name, "CMOR_EXIT_ON_WARNING") == 0) {
        return (Py_BuildValue("i", CMOR_EXIT_ON_WARNING));
    } else if (strcmp(att_name, "CMOR_VERSION_MAJOR") == 0) {
        return (Py_BuildValue("i", CMOR_VERSION_MAJOR));
    } else if (strcmp(att_name, "CMOR_VERSION_MINOR") == 0) {
        return (Py_BuildValue("i", CMOR_VERSION_MINOR));
    } else if (strcmp(att_name, "CMOR_VERSION_PATCH") == 0) {
        return (Py_BuildValue("i", CMOR_VERSION_PATCH));
    } else if (strcmp(att_name, "CMOR_CF_VERSION_MAJOR") == 0) {
        return (Py_BuildValue("i", CMOR_CF_VERSION_MAJOR));
    } else if (strcmp(att_name, "CMOR_CF_VERSION_MINOR") == 0) {
        return (Py_BuildValue("i", CMOR_CF_VERSION_MINOR));
    } else if (strcmp(att_name, "CMOR_WARNING") == 0) {
        return (Py_BuildValue("i", CMOR_WARNING));
    } else if (strcmp(att_name, "CMOR_NORMAL") == 0) {
        return (Py_BuildValue("i", CMOR_NORMAL));
    } else if (strcmp(att_name, "CMOR_CRITICAL") == 0) {
        return (Py_BuildValue("i", CMOR_CRITICAL));
    } else if (strcmp(att_name, "CMOR_N_VALID_CALS") == 0) {
        return (Py_BuildValue("i", CMOR_N_VALID_CALS));
    } else if (strcmp(att_name, "CMOR_PRESERVE") == 0) {
        return (Py_BuildValue("i", CMOR_PRESERVE));
    } else if (strcmp(att_name, "CMOR_APPEND") == 0) {
        return (Py_BuildValue("i", CMOR_APPEND));
    } else if (strcmp(att_name, "CMOR_REPLACE") == 0) {
        return (Py_BuildValue("i", CMOR_REPLACE));
    } else if (strcmp(att_name, "CMOR_PRESERVE_3") == 0) {
        return (Py_BuildValue("i", CMOR_PRESERVE_3));
    } else if (strcmp(att_name, "CMOR_APPEND_3") == 0) {
        return (Py_BuildValue("i", CMOR_APPEND_3));
    } else if (strcmp(att_name, "CMOR_REPLACE_3") == 0) {
        return (Py_BuildValue("i", CMOR_REPLACE_3));
    } else if (strcmp(att_name, "CMOR_PRESERVE_4") == 0) {
        return (Py_BuildValue("i", CMOR_PRESERVE_4));
    } else if (strcmp(att_name, "CMOR_APPEND_4") == 0) {
        return (Py_BuildValue("i", CMOR_APPEND_4));
    } else if (strcmp(att_name, "CMOR_REPLACE_4") == 0) {
        return (Py_BuildValue("i", CMOR_REPLACE_4));
    } else if (strcmp(att_name, "TABLE_CONTROL_FILENAME") == 0) {
        return (Py_BuildValue("s", TABLE_CONTROL_FILENAME));
    } else if (strcmp(att_name, "GLOBAL_CV_FILENAME") == 0) {
        return (Py_BuildValue("s", GLOBAL_CV_FILENAME));
    } else if (strcmp(att_name, "CMOR_DEFAULT_FURTHERURL_TEMPLATE") == 0) {
        return (Py_BuildValue("s", CMOR_DEFAULT_FURTHERURL_TEMPLATE));
    } else if (strcmp(att_name, "FILE_PATH_TEMPLATE") == 0) {
        return (Py_BuildValue("s", FILE_PATH_TEMPLATE));
    } else if (strcmp(att_name, "FILE_NAME_TEMPLATE") == 0) {
        return (Py_BuildValue("s", FILE_NAME_TEMPLATE));
    } else if (strcmp(att_name, "CV_CHECK_ERROR") == 0) {
        return (Py_BuildValue("s", CV_CHECK_ERROR));
    } else if (strcmp(att_name, "GLOBAL_ATT_FURTHERINFOURLTMPL") == 0) {
        return (Py_BuildValue("s", GLOBAL_ATT_FURTHERINFOURLTMPL));
    } else if (strcmp(att_name, "GLOBAL_ATT_MEMBER_ID") == 0) {
        return (Py_BuildValue("s", GLOBAL_ATT_MEMBER_ID));
    } else if (strcmp(att_name, "CMOR_AXIS_ENTRY_FILE") == 0) {
        return (Py_BuildValue("s", CMOR_AXIS_ENTRY_FILE));
    } else if (strcmp(att_name, "CMOR_FORMULA_VAR_FILE") == 0) {
        return (Py_BuildValue("s", CMOR_FORMULA_VAR_FILE));

    } else {
        /* Return NULL Python Object */
        Py_INCREF(Py_None);
        return (Py_None);
    }
}

/************************************************************************/
/*                       PyCV_setup_variable()                          */
/************************************************************************/
static PyObject *PyCV_setup_variable(PyObject * self, PyObject * args)
{
    char *name;
    char *units;
    float missing;
    int imissing;
    double startime;
    double endtime;
    double startimebnds;
    double endtimebnds;

    int var_id;

    if (!PyArg_ParseTuple(args, "ssfidddd", &name, &units, 
                          &missing, &imissing,
                          &startime, &endtime, 
                          &startimebnds, &endtimebnds)) {
        return (Py_BuildValue("i", -1));
    }

    cmor_CV_variable(&var_id, name, units, &missing, &imissing, 
                     startime, endtime, startimebnds, endtimebnds);

    return (Py_BuildValue("i", var_id));

}

/************************************************************************/
/*                          PyCMOR_set_table()                          */
/************************************************************************/

static PyObject *PyCMOR_set_table(PyObject * self, PyObject * args)
{
    int table, ierr;
    cmor_is_setup();

    if (!PyArg_ParseTuple(args, "i", &table))
        return NULL;
    ierr = cmor_set_table(table);
    if (ierr != 0)
        return NULL;
/* -------------------------------------------------------------------- */
/*      Return NULL Python Object                                       */
/* -------------------------------------------------------------------- */

    Py_INCREF(Py_None);
    return (Py_None);
}

static PyMethodDef MyExtractMethods[] = {
    {"setup", PyCMOR_setup, METH_VARARGS},
    {"load_table", PyCMOR_load_table, METH_VARARGS},
    {"set_table", PyCMOR_set_table, METH_VARARGS},
    {"set_cur_dataset_attribute", PyCMOR_set_cur_dataset_attribute,
     METH_VARARGS},
    {"get_cur_dataset_attribute", PyCMOR_get_cur_dataset_attribute,
     METH_VARARGS},
    {"has_cur_dataset_attribute", PyCMOR_has_cur_dataset_attribute,
     METH_VARARGS},
    {"set_variable_attribute", PyCMOR_set_variable_attribute,
     METH_VARARGS},
    {"get_variable_attribute", PyCMOR_get_variable_attribute,
     METH_VARARGS},
    {"has_variable_attribute", PyCMOR_has_variable_attribute,
     METH_VARARGS},
    {"list_variable_attributes", PyCMOR_get_variable_attribute_list,
     METH_VARARGS},
    {"set_institution", PyCV_setInstitution, METH_VARARGS},
    {"check_parentExpID", PyCV_checkParentExpID, METH_VARARGS},
    {"check_subExpID", PyCV_checkSubExpID, METH_VARARGS},
    {"check_sourceID", PyCV_checkSourceID, METH_VARARGS},
    {"check_grids", PyCV_checkGrids, METH_VARARGS},
    {"check_experiment", PyCV_checkExperiment, METH_VARARGS},
    {"check_furtherinfourl", PyCV_checkFurtherInfoURL, METH_VARARGS},
    {"check_gblattributes", PyCV_GblAttributes, METH_VARARGS},
    {"check_filename", PyCV_checkFilename, METH_VARARGS},
    {"check_ISOTime", PyCV_checkISOTime, METH_VARARGS},
    {"getCMOR_defaults_include", PyCMOR_getincvalues, METH_VARARGS},
    {"setup_variable", PyCV_setup_variable, METH_VARARGS},
    {"get_CV_Error", PyCV_get_Error, METH_VARARGS},
    {"reset_CV_Error", PyCV_reset_Error},
    {"set_CV_Error", PyCV_set_Error, METH_VARARGS},

    {NULL, NULL}                /*sentinel */
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_cmip6_cv",
        NULL,
        -1,
        MyExtractMethods
};
#endif


#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit__cmip6_cv(void)
{
    PyObject *cmip6_cv_module;
    cmip6_cv_module = PyModule_Create(&moduledef);
    import_array();
    return cmip6_cv_module;
}
#else
void init_cmip6_cv(void)
{
    (void)Py_InitModule("_cmip6_cv", MyExtractMethods);
    import_array();
}
#endif
