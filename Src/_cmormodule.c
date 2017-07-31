#include <Python.h>
#define NPY_NO_DEPRECATED_API  NPY_1_10_API_VERSION
#include "numpy/arrayobject.h"
#include "cmor.h"

static PyObject *CMORError;
volatile sig_atomic_t raise_exception = 0;
char *exception_message = "Problem with 'cmor.%s'. Please check the logfile "
  "(if defined).";
int signal_to_catch = SIGTERM;

void signal_handler(int signal)
{
    if (signal == signal_to_catch) {
        raise_exception = 1;
    }
}

/************************************************************************/
/*                     PyCMOR_get_original_shape()                      */
/************************************************************************/

static PyObject *PyCMOR_get_original_shape(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int i, shape_array[CMOR_MAX_DIMENSIONS], var_id, blank_time;

    i = CMOR_MAX_DIMENSIONS;
    PyObject *mylist;

    if (!PyArg_ParseTuple(args, "ii", &var_id, &blank_time))
        return NULL;
    cmor_get_original_shape(&var_id, &shape_array[0], &i, blank_time);

    mylist = PyList_New(0);
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        if (shape_array[i] != -1) {
            PyList_Append(mylist, PyInt_FromLong(shape_array[i]));
        }
    }
    Py_INCREF(mylist);

    if (raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "get_original_shape");
        return NULL;
    }

    return (mylist);
}

/************************************************************************/
/*                  PyCMOR_set_cur_dataset_attribute()                  */
/************************************************************************/
static PyObject *PyCMOR_set_cur_dataset_attribute(PyObject * self,
                                                  PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    char *name;
    char *value;
    int ierr;

    if (!PyArg_ParseTuple(args, "ss", &name, &value))
        return (NULL);

    ierr = cmor_set_cur_dataset_attribute(name, value, 1);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "set_cur_dataset_attribute");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                  PyCMOR_get_cur_dataset_attribute()                  */
/************************************************************************/
static PyObject *PyCMOR_get_cur_dataset_attribute(PyObject * self,
                                                  PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    char *name;
    char value[CMOR_MAX_STRING];
    int ierr;

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    ierr = cmor_get_cur_dataset_attribute(name, value);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "get_cur_dataset_attribute");
        return NULL;
    }

    return (Py_BuildValue("s", value));
}

/************************************************************************/
/*                  PyCMOR_has_cur_dataset_attribute()                  */
/************************************************************************/
static PyObject *PyCMOR_has_cur_dataset_attribute(PyObject * self,
                                                  PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    char *name;
    int ierr;

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    ierr = cmor_has_cur_dataset_attribute(name);

    if (raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "has_cur_dataset_attribute");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                   PyCMOR_set_deflate()                    */
/************************************************************************/
static PyObject *PyCMOR_set_deflate(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, var_id, shuffle, deflate, deflate_level;

    if (!PyArg_ParseTuple
        (args, "iiii", &var_id, &shuffle, &deflate, &deflate_level))
        return NULL;

    ierr = cmor_set_deflate(var_id, shuffle, deflate, deflate_level);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "set_deflate");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                   PyCMOR_set_variable_attribute()                    */
/************************************************************************/
static PyObject *PyCMOR_set_variable_attribute(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    char *name;
    char *value;
    int ierr, var_id;

    if (!PyArg_ParseTuple(args, "iss", &var_id, &name, &value))
        return NULL;

    ierr = cmor_set_variable_attribute(var_id, name, 'c', (void *)value);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "set_variable_attribute");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                   PyCMOR_get_variable_attribute()                    */
/************************************************************************/
static PyObject *PyCMOR_get_variable_attribute(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    char *name;
    char value[CMOR_MAX_STRING];
    int ierr, var_id;

    if (!PyArg_ParseTuple(args, "is", &var_id, &name))
        return NULL;

    ierr = cmor_get_variable_attribute(var_id, name, (void *)value);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "get_variable_attribute");
        return NULL;
    }

    return (Py_BuildValue("s", value));
}

/************************************************************************/
/*                   PyCMOR_has_variable_attribute()                    */
/************************************************************************/
static PyObject *PyCMOR_has_variable_attribute(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    char *name;
    int ierr, var_id;

    if (!PyArg_ParseTuple(args, "is", &var_id, &name))
        return NULL;

    ierr = cmor_has_variable_attribute(var_id, name);

    if (raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "has_variable_attribute");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                            PyCMOR_setup()                            */
/************************************************************************/
static PyObject *PyCMOR_setup(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
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

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "setup");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
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
    } else {
        /* Return NULL Python Object */
        Py_INCREF(Py_None);
        return (Py_None);
    }
}

/************************************************************************/
/*                       PyCMOR_getFinalFilename()                      */
/************************************************************************/

static PyObject *PyCMOR_getFinalFilename(PyObject * self, PyObject * args)
{
    return (Py_BuildValue("s", cmor_getFinalFilename()));

}

/************************************************************************/
/*                       PyCMOR_dataset_json()                          */
/************************************************************************/

static PyObject *PyCMOR_dataset_json(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr;
    char *rcfile;

    if (!PyArg_ParseTuple(args, "s", &rcfile)) {
        return (Py_BuildValue("i", -1));
    }

    ierr = cmor_dataset_json(rcfile);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "dataset_json");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                         PyCMOR_load_table()                          */
/************************************************************************/

static PyObject *PyCMOR_load_table(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, table_id;
    char *table;

    if (!PyArg_ParseTuple(args, "s", &table))
        return NULL;

    ierr = cmor_load_table(table, &table_id);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "load_table");
        return NULL;
    }

    return (Py_BuildValue("i", table_id));
}

/************************************************************************/
/*                            PyCMOR_axis()                             */
/************************************************************************/

static PyObject *PyCMOR_axis(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, axis_id, n = 0;
    char *name;
    char *units;
    char *interval;
    int length;
    char type;
    void *coord_vals;
    void *cell_bounds;
    int cell_bounds_ndim;
    char *tmpstr = NULL;
    PyObject *coords_obj, *bounds_obj;
    PyArrayObject *coords = NULL, *bounds = NULL;

/************************************************************************/
/*         HUGE assumtion here is that the data is contiguous!          */
/************************************************************************/

    if (!PyArg_ParseTuple
        (args, "ssiOcOis", &name, &units, &length, &coords_obj, &type,
         &bounds_obj, &cell_bounds_ndim, &interval))
        return NULL;

    if (coords_obj == Py_None) {
        coord_vals = NULL;
    } else {
        coords =
          (PyArrayObject *) PyArray_ContiguousFromObject(coords_obj,
                                                         NPY_NOTYPE, 1, 0);

        if (PyArray_NDIM(coords) != 1) {
            printf("ok we need to pass contiguous flattened arrays only!\n");
            return NULL;
        }

        if (type != 'c') {
            coord_vals = (void *)PyArray_DATA(coords);
            n = cell_bounds_ndim;
        } else {
            tmpstr =
              (char *)malloc(sizeof(char) * length * (cell_bounds_ndim + 1));
            for (ierr = 0; ierr < length; ierr++) {
                coord_vals = (void *)PyArray_GETPTR1(coords, ierr);
                strncpy(&tmpstr[ierr * (cell_bounds_ndim + 1)],
                        coord_vals, cell_bounds_ndim);
                tmpstr[ierr * (cell_bounds_ndim + 1) + cell_bounds_ndim] = '\0';
            }
            coord_vals = &tmpstr[0];
            n = cell_bounds_ndim + 1;
            for (ierr = 0; ierr < length; ierr++) {
            }
        }
    }

    if (bounds_obj == Py_None) {
        cell_bounds = NULL;
    } else {
        bounds =
          (PyArrayObject *) PyArray_ContiguousFromObject(bounds_obj,
                                                         NPY_NOTYPE, 1, 0);
        if (PyArray_NDIM(bounds) != 1) {
            printf("ok we need to pass contiguous flattened arrays only!\n");
            return NULL;
        }
        cell_bounds = (void *)PyArray_DATA(bounds);
    }

    ierr =
      cmor_axis(&axis_id, name, units, length, coord_vals, type,
                cell_bounds, n, interval);

    if (coords != NULL) {
        Py_DECREF(coords);
    }
    if (bounds != NULL) {
        Py_DECREF(bounds);
    }

    if (type == 'c') {
        free(tmpstr);
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "axis");
        return NULL;
    }

    return (Py_BuildValue("i", axis_id));
}

/************************************************************************/
/*                          PyCMOR_set_table()                          */
/************************************************************************/

static PyObject *PyCMOR_set_table(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int table, ierr;

    if (!PyArg_ParseTuple(args, "i", &table))
        return NULL;

    ierr = cmor_set_table(table);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "set_table");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

static PyObject *PyCMOR_variable(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, var_id;
    char *name;
    char *units;
    char *comment;
    char *positive;
    char *original_name;
    char *history;
    int ndims;
    char type;
    double missing;
    float fmissing;
    int imissing;
    long lmissing;
    double tol;
    void *pass_missing = NULL;
    int *axes_ids;
    PyObject *axes_obj, *missing_obj;
    PyArrayObject *axes = NULL;

    /* HUGE assumtion here is that the data is contiguous! */
    if (!PyArg_ParseTuple
        (args, "ssiOcOdssss", &name, &units, &ndims, &axes_obj, &type,
         &missing_obj, &tol, &positive, &original_name, &history, &comment))
        return NULL;

    axes =
      (PyArrayObject *) PyArray_ContiguousFromObject(axes_obj,
                                                     NPY_NOTYPE, 1, 0);

    if (PyArray_NDIM(axes) != 1) {
        printf("ok we need to pass contiguous flattened arrays only!\n");
        return NULL;
    }
    axes_ids = (int *)PyArray_DATA(axes);

    if (missing_obj == Py_None) {
        pass_missing = NULL;
    } else {
        missing = PyFloat_AsDouble(missing_obj);
        if (type == 'd')
            pass_missing = &missing;
        else if (type == 'f') {
            fmissing = (float)missing;
            pass_missing = &fmissing;
        } else if (type == 'l') {
            lmissing = (long)missing;
            pass_missing = &lmissing;
        } else if (type == 'i') {
            imissing = (int)missing;
            pass_missing = &imissing;
        }
    }

    ierr =
      cmor_variable(&var_id, name, units, ndims, axes_ids, type,
                    pass_missing, &tol, positive, original_name,
                    history, comment);
    if (axes != NULL) {
        Py_DECREF(axes);
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "variable");
        return NULL;
    }

    return (Py_BuildValue("i", var_id));
}

static PyObject *PyCMOR_zfactor(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, zvar_id;
    int itmp;
    int axis_id;
    char *name;
    char *units;
    int ndims;
    char type;
    int *axes_ids;
    void *values, *bounds;
    PyObject *axes_obj, *values_obj, *bounds_obj;
    PyArrayObject *axes = NULL, *values_array = NULL, *bounds_array = NULL;

/* -------------------------------------------------------------------- */
/*      HUGE assumtion here is that the data is contiguous!             */
/* -------------------------------------------------------------------- */

    if (!PyArg_ParseTuple
        (args, "issiOcOO", &axis_id, &name, &units, &ndims, &axes_obj,
         &type, &values_obj, &bounds_obj))
        return NULL;

    if (axes_obj == Py_None) {
        axes_ids = NULL;
    } else {
        if (ndims > 1) {
            axes =
              (PyArrayObject *) PyArray_ContiguousFromObject(axes_obj,
                                                             NPY_NOTYPE, 1, 0);
            axes_ids = (void *)PyArray_DATA(axes);
        } else {
            itmp = (int)PyInt_AsLong(axes_obj);
            axes_ids = &itmp;
        }
    }

    if (values_obj == Py_None) {
        values = NULL;
    } else {
        values_array =
          (PyArrayObject *) PyArray_ContiguousFromObject(values_obj,
                                                         NPY_NOTYPE, 1, 0);
        values = (void *)PyArray_DATA(values_array);
    }

    if (bounds_obj == Py_None) {
        bounds = NULL;
    } else {
        bounds_array =
          (PyArrayObject *) PyArray_ContiguousFromObject(bounds_obj,
                                                         NPY_NOTYPE, 1, 0);
        bounds = (void *)PyArray_DATA(bounds_array);
    }

    ierr =
      cmor_zfactor(&zvar_id, axis_id, name, units, ndims, axes_ids,
                   type, values, bounds);
    if (axes != NULL) {
        Py_DECREF(axes);
    }
    if (values_array != NULL) {
        Py_DECREF(values_array);
    }
    if (bounds_array != NULL) {
        Py_DECREF(bounds_array);
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "zfactor");
        return NULL;
    }

    return (Py_BuildValue("i", zvar_id));
}

static PyObject *PyCMOR_grid_mapping(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr;
    PyObject *param_nm_obj, *param_val_obj, *param_un_obj, *tmp;
    PyArrayObject *param_val_arr = NULL;
    void *param_val;
    char *name;
    int gid, i, n;
    char nms[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    char units[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];

    /* HUGE assumtion here is that the data is contiguous! */
    if (!PyArg_ParseTuple
        (args, "isOOO", &gid, &name, &param_nm_obj, &param_val_obj,
         &param_un_obj))
        return NULL;

    param_val_arr =
      (PyArrayObject *) PyArray_ContiguousFromObject(param_val_obj,
                                                     NPY_NOTYPE, 1, 0);
    param_val = PyArray_DATA(param_val_arr);

    n = PyList_Size(param_nm_obj);
    for (i = 0; i < n; i++) {
        tmp = PyList_GetItem(param_nm_obj, i);
        strcpy(nms[i], PyString_AsString(tmp));
        //Py_DECREF(tmp); //Not needed get_item does not increase ref
        tmp = PyList_GetItem(param_un_obj, i);
        strcpy(units[i], PyString_AsString(tmp));
        //Py_DECREF(tmp); // Not need get_item does not incref
    }

    ierr =
      cmor_set_grid_mapping(gid, name, n, (char *)nms,
                            CMOR_MAX_STRING, param_val,
                            (char *)units, CMOR_MAX_STRING);

    if (param_val_arr != NULL) {
        Py_DECREF(param_val_arr);
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "grid_mapping");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                            PyCMOR_write()                            */
/************************************************************************/

static PyObject *PyCMOR_write(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, var_id;
    PyObject *data_obj = NULL;
    PyArrayObject *data_array = NULL;
    void *data;
    char *itype;
    char type;
    int ntimes;
    PyObject *times_obj = NULL;
    PyArrayObject *times_array = NULL;
    void *times;
    double itime;
    PyObject *times_bnds_obj = NULL;
    PyArrayObject *times_bnds_array = NULL;
    void *times_bnds;
    PyObject *ref_obj;
    int *ref;
    int iref;

    if (!PyArg_ParseTuple
        (args, "iOsiOOO", &var_id, &data_obj, &itype, &ntimes,
         &times_obj, &times_bnds_obj, &ref_obj))
        return NULL;

    data_array =
      (PyArrayObject *) PyArray_ContiguousFromObject(data_obj,
                                                     NPY_NOTYPE, 1, 0);
    data = PyArray_DATA(data_array);

    if (times_obj == Py_None) {
        times = NULL;
    } else {
        if (ntimes > 1) {
            times_array = (PyArrayObject *)
              PyArray_ContiguousFromObject(times_obj, NPY_NOTYPE, 1, 0);
            times = (void *)PyArray_DATA(times_array);
        } else {
            itime = (double)PyFloat_AsDouble(times_obj);
            times = &itime;
        }
    }

    if (times_bnds_obj == Py_None) {
        times_bnds = NULL;
    } else {
        times_bnds_array = (PyArrayObject *)
          PyArray_ContiguousFromObject(times_bnds_obj, NPY_NOTYPE, 1, 0);
        times_bnds = (void *)PyArray_DATA(times_bnds_array);
    }

    if (ref_obj == Py_None) {
        ref = NULL;
    } else {
        iref = (int)PyInt_AsLong(ref_obj);
        ref = &iref;
    }
    type = itype[0];

    ierr = 0;
    ierr = cmor_write(var_id, data, type, ntimes, times, times_bnds, ref);
    Py_DECREF(data_array);
    if (times_array != NULL) {
        Py_DECREF(times_array);
    }
    if (times_bnds_array != NULL) {
        Py_DECREF(times_bnds_array);
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "write");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}

/************************************************************************/
/*                            PyCMOR_close()                            */
/************************************************************************/

static PyObject *PyCMOR_close(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    PyObject *var;
    int varid, ierr;
    int dofile = 0;
    int dopreserve = 0;
    int preserved_id;
    char file_name[CMOR_MAX_STRING];

    if (!PyArg_ParseTuple(args, "Oii", &var, &dofile, &dopreserve)) {
        return NULL;
    }

    if (var == Py_None) {
        ierr = cmor_close();
        if (ierr != 0) {
            return NULL;
        } else {
            return (Py_BuildValue("i", ierr));
        }
    } else {
        varid = (int)PyInt_AsLong(var);

        if (dopreserve == 1) {
            if (dofile == 1) {
                ierr = cmor_close_variable(varid, &file_name[0], &preserved_id);
            } else {
                ierr = cmor_close_variable(varid, NULL, &preserved_id);
            }
        } else {
            if (dofile == 1) {
                ierr = cmor_close_variable(varid, &file_name[0], NULL);
            } else {
                ierr = cmor_close_variable(varid, NULL, NULL);
            }
        }
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "close");
        return NULL;
    } else {
        if (dofile == 1) {
            return (Py_BuildValue("s", file_name));
        } else {
            return (Py_BuildValue("i", ierr));
        }
    }
}

/************************************************************************/
/*                PyCMOR_time_varying_grid_coordinate()                 */
/************************************************************************/

static PyObject *PyCMOR_time_varying_grid_coordinate(PyObject * self,
                                                     PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr, grid_id, coord_var_id;
    char *table_entry;
    char *units;
    char type;
    double missing;
    PyObject *missing_obj;
    void *pass_missing;

    /* HUGE assumtion here is that the data is contiguous! */
    if (!PyArg_ParseTuple
        (args, "isscO", &grid_id, &table_entry, &units, &type, &missing_obj))
        return NULL;
    if (missing_obj == Py_None) {
        pass_missing = NULL;
    } else {
        missing = PyFloat_AsDouble(missing_obj);
        pass_missing = (void *)&missing;
    }
    ierr =
      cmor_time_varying_grid_coordinate(&coord_var_id, grid_id,
                                        table_entry, units, type,
                                        pass_missing, NULL);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message,
                     "time_varying_grid_coordinate");
        return NULL;
    }

    return (Py_BuildValue("i", coord_var_id));
}

/************************************************************************/
/*                            PyCMOR_grid()                             */
/************************************************************************/
static PyObject *PyCMOR_set_furtherinfourl(PyObject * self, PyObject * args) 
{
    int varid;
    int ierr;

    signal(signal_to_catch, signal_handler);

    if (!PyArg_ParseTuple (args, "i", &varid)) {
        return NULL;
    }

    ierr = cmor_CV_checkFurtherInfoURL(varid);

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "set_futherurlinfo");
        return NULL;
    }

    return (Py_BuildValue("i", ierr));
}
/************************************************************************/
/*                            PyCMOR_grid()                             */
/************************************************************************/

static PyObject *PyCMOR_grid(PyObject * self, PyObject * args)
{
    signal(signal_to_catch, signal_handler);
    int ierr;
    PyObject *axes_obj, *lat_obj, *lon_obj, *blat_obj, *blon_obj;
    PyArrayObject *axes_arr = NULL, *lat_arr = NULL, *lon_arr = NULL,
      *blat_arr = NULL, *blon_arr = NULL;
    void *axes, *lon, *lat, *blon, *blat;
    char type;
    int nvert, ndims;
    int id;
    char itype;

    if (!PyArg_ParseTuple
        (args, "iOcOOiOO", &ndims, &axes_obj, &itype, &lat_obj, &lon_obj,
         &nvert, &blat_obj, &blon_obj))
        return NULL;

    type = itype;
    axes_arr =
      (PyArrayObject *) PyArray_ContiguousFromObject(axes_obj,
                                                     NPY_NOTYPE, 1, 0);
    axes = (void *)PyArray_DATA(axes_arr);

    if (lat_obj == Py_None) {
        lat = NULL;
    } else {
        lat_arr =
          (PyArrayObject *) PyArray_ContiguousFromObject(lat_obj,
                                                         NPY_NOTYPE, 1, 0);
        lat = (void *)PyArray_DATA(lat_arr);
    }

    if (lon_obj == Py_None) {
        lon = NULL;
    } else {
        lon_arr =
          (PyArrayObject *) PyArray_ContiguousFromObject(lon_obj,
                                                         NPY_NOTYPE, 1, 0);
        lon = (void *)PyArray_DATA(lon_arr);
    }

    if (blat_obj == Py_None) {
        blat = NULL;
    } else {
        blat_arr =
          (PyArrayObject *) PyArray_ContiguousFromObject(blat_obj,
                                                         NPY_NOTYPE, 1, 0);
        blat = (void *)PyArray_DATA(blat_arr);
    }
    if (blon_obj == Py_None) {
        blon = NULL;
    } else {
        blon_arr =
          (PyArrayObject *) PyArray_ContiguousFromObject(blon_obj,
                                                         NPY_NOTYPE, 1, 0);
        blon = (void *)PyArray_DATA(blon_arr);
    }

    ierr = cmor_grid(&id, ndims, axes, type, lat, lon, nvert, blat, blon);

    if (axes_arr != NULL) {
        Py_DECREF(axes_arr);
    }
    if (lat_arr != NULL) {
        Py_DECREF(lat_arr);
    }
    if (blat_arr != NULL) {
        Py_DECREF(blat_arr);
    }
    if (lon_arr != NULL) {
        Py_DECREF(lon_arr);
    }
    if (blon_arr != NULL) {
        Py_DECREF(blon_arr);
    }

    if (ierr != 0 || raise_exception) {
        raise_exception = 0;
        PyErr_Format(CMORError, exception_message, "grid");
        return NULL;
    }
    return (Py_BuildValue("i", id));
}

static PyMethodDef MyExtractMethods[] = {
    {"setup", PyCMOR_setup, METH_VARARGS},
    {"dataset_json", PyCMOR_dataset_json, METH_VARARGS},
    {"load_table", PyCMOR_load_table, METH_VARARGS},
    {"axis", PyCMOR_axis, METH_VARARGS},
    {"set_table", PyCMOR_set_table, METH_VARARGS},
    {"variable", PyCMOR_variable, METH_VARARGS},
    {"zfactor", PyCMOR_zfactor, METH_VARARGS},
    {"write", PyCMOR_write, METH_VARARGS},
    {"grid", PyCMOR_grid, METH_VARARGS},
    {"time_varying_grid_coordinate", PyCMOR_time_varying_grid_coordinate,
     METH_VARARGS},
    {"set_grid_mapping", PyCMOR_grid_mapping, METH_VARARGS},
    {"getCMOR_defaults_include", PyCMOR_getincvalues, METH_VARARGS},
    {"close", PyCMOR_close, METH_VARARGS},
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
    {"get_original_shape", PyCMOR_get_original_shape, METH_VARARGS},
    {"set_furtherinfourl", PyCMOR_set_furtherinfourl, METH_VARARGS},
    {"get_final_filename", PyCMOR_getFinalFilename, METH_VARARGS},
    {"set_deflate", PyCMOR_set_deflate, METH_VARARGS},
    {NULL, NULL}                /*sentinel */
};

PyMODINIT_FUNC init_cmor(void)
{
    PyObject *cmor_module;
    cmor_module = Py_InitModule("_cmor", MyExtractMethods);
    import_array();
    CMORError = PyErr_NewException("_cmor.CMORError", NULL, NULL);
    PyModule_AddObject(cmor_module, "CMORError", CMORError);
}
