# -*- coding: utf-8 -*-
'''
Created on Fri Feb 19 11:33:52 2016

@author: Denis Nadeau LLNL, Charles Doutriaux LLNL
'''

from cmip6_cv import cmor_const
import numpy
import os
from cmip6_cv import _cmip6_cv
import signal
import six


def sig_handler(signum, frame):
    os.kill(os.getpid(), signal.SIGABRT)


signal.signal(signal.SIGTERM, sig_handler)
signal.signal(signal.SIGINT, sig_handler)

try:
    import cdtime
    has_cdtime = True
except BaseException:
    has_cdtime = False

# try:
    #import cdms2
    #has_cdms2 = True
# except BaseException:
    #has_cdms2 = False

# try:
    #import MV2
    #has_MV2 = True
# except BaseException:
    #has_MV2 = False

try:
    import numpy.oldnumeric.ma.MaskedArray
    has_oldma = True
except BaseException:
    has_oldma = False


def _to_numpy(vals, message):
    if isinstance(vals, (list, tuple)):
        vals = numpy.ascontiguousarray(vals)
    elif not isinstance(vals, numpy.ndarray):
        try:
            vals = numpy.ascontiguousarray(vals.filled())
        except BaseException:
            raise Exception(
                "Error could not convert %s to a numpy array" %
                message)

    return vals


def setup(inpath='.',
          set_verbosity=cmor_const.CMOR_NORMAL, exit_control=cmor_const.CMOR_NORMAL,
          logfile=None, create_subdirectories=1):
    """
    Usage cmor_setup(inpath='.',netcdf_file_action=cmor.CMOR_PRESERVE,
                     set_verbosity=cmor.CMOR_NORMAL,exit_control=cmor.CMOR_NORMAL)
    Where:
    path:                  Alternate directory where to find tables if not in current directory
    set_verbosity:         CMOR_QUIET or CMOR_NORMAL
    exit_control:          CMOR_EXIT_ON_WARNING, CMOR_EXIT_ON_MAJOR, CMOR_NORMAL
    create_subdirectories: 1 to create subdirectories structure, 0 to dump files to the directory specified by the "outpath" attribute 
                           in the user input JSON file passed to cmor_dataset_json
"""
    if not isinstance(exit_control, int) or not exit_control in [cmor_const.CMOR_EXIT_ON_WARNING,
                                                                 cmor_const.CMOR_EXIT_ON_MAJOR,
                                                                 cmor_const.CMOR_NORMAL]:
        raise Exception("exit_control must an integer valid values are: CMOR_EXIT_ON_WARNING, "
                        "CMOR_EXIT_ON_MAJOR, CMOR_NORMAL")

    if not isinstance(set_verbosity, int) or not set_verbosity in [
            cmor_const.CMOR_QUIET, cmor_const.CMOR_NORMAL]:
        raise Exception(
            "set_verbosity must an integer valid values are: CMOR_QUIET, CMOR_NORMAL")

    if not isinstance(inpath, str) and not os.path.exists(inpath):
        raise Exception("path must be a Valid path")
    if logfile is None:
        logfile = ""

    if create_subdirectories not in [0, 1]:
        raise Exception("create_subdirectories must be 0 or 1")
    CMOR_PRESERVE = 10
    return(_cmip6_cv.setup(inpath, CMOR_PRESERVE, set_verbosity, exit_control, logfile, create_subdirectories))


def load_table(table):
    """ loads a cmor table
    Usage:
    load_table(table)
    """
    if not isinstance(table, six.string_types):
        raise Exception("Error, must pass a string")
#     if not os.path.exists(table):
#         raise Exception, "Error, the table you specified (%s) does not exists" % table
    return _cmip6_cv.load_table(table)


def set_table(table):
    if not isinstance(table, int):
        raise Exception("error you need to pass and integer as the table id")
    return(_cmip6_cv.set_table(table))


def close(var_id=None, file_name=False, preserve=False):
    """ Close CMOR variables/file
    Usage:
      cmor.close(varid=None)
    Where:
      var_id: id of variable to close, if passing None, means close every open ones.
      [file_name] True/False (default False) if True: return name of the file just closed,
                  works only if var_id is not None
      [preserve] True/False (default False) if True: close the file but preserve the var
                  definition in CMOR to write more data with this variable (into a new file)
      """
    if var_id is not None and not isinstance(var_id, int):
        raise Exception("Error var_id must be None or a integer")

    if (preserve is False):
        if (file_name is False):
            return(_cmip6_cv.close(var_id, 0, 0))
        else:
            return(_cmip6_cv.close(var_id, 1, 0))
    else:
        if (file_name is False):
            return(_cmip6_cv.close(var_id, 0, 1))
        else:
            return(_cmip6_cv.close(var_id, 1, 1))


def set_cur_dataset_attribute(name, value):
    """Sets an attribute onto the current cmor dataset
    Usage:
      cmor.set_cur_dataset_attribute(name,value)
    Where:
      name: is the name of the attribute
      value: is the value for this attribute
    """
    if value is None:
        val = ""
    else:
        val = str(value)
    return(_cmip6_cv.set_cur_dataset_attribute(name, val))


def has_cur_dataset_attribute(name):
    """determines if the current cmor dataset has an attribute
    Usage:
      cmor.has_cur_dataset_attribute(name)
    Where:
      name: is the name of the attribute
    Returns True if the dataset has the attribute, False otherwise
    """
    test = _cmip6_cv.has_cur_dataset_attribute(name)
    if test == 0:
        return True
    else:
        return False


def get_cur_dataset_attribute(name):
    """Gets an attribute from the current cmor dataset
    Usage:
      cmor.get_cur_dataset_attribute(name)
    Where:
      name: is the name of the attribute
    Returns none if attribute is non-existant
    """
    if has_cur_dataset_attribute(name):
        return _cmip6_cv.get_cur_dataset_attribute(name)
    else:
        return None


def set_variable_attribute(var_id, name, value):
    """Sets an attribute onto a cmor variable
    Usage:
      cmor.set_variable_attribute(var_id,name,value)
    Where:
      var_id: is cmor variable id
      name  : is the name of the attribute
      value : is the value for this attribute
    """
    if value is None:
        val = ""
    else:
        val = str(value)
    return _cmip6_cv.set_variable_attribute(var_id, name, val)


def has_variable_attribute(var_id, name):
    """determines if the a cmor variable has an attribute
    Usage:
      cmor.het_variable_attribute(name)
    Where:
      var_id: is cmor variable id
      name: is the name of the attribute
    Returns True if the dataset has the attribute, False otherwise
    """
    test = _cmip6_cv.has_variable_attribute(var_id, name)
    if test == 0:
        return True
    else:
        return False


def list_variable_attributes(var_id):
    """List all attribute from a cmor variable
    Usage:
      cmip6_cv.list_variable_attribute(var_id)
    Where:
      var_id: is cmor variable id
    Returns none if the variable as no attribute
    """
    var_list = _cmip6_cv.list_variable_attributes(var_id)
    return var_list


def get_variable_attribute(var_id, name):
    """Gets an attribute from a cmor variable
    Usage:
      cmor.get_variable_attribute(name)
    Where:
      var_id: is cmor variable id
      name: is the name of the attribute
    Returns none if attribute is non-existant
    """
# print 'In there asking for attribute: ',name,'on var',var_id
    if has_variable_attribute(var_id, name):
        # print 'Seems to have it',var_id,name
        return _cmip6_cv.get_variable_attribute(var_id, name)
    else:
        return None


def check_institution(table_id):
    '''
        Verify institution against Control Vocabulary files
        Usage:
          cmip6_cv.set_institution(table_id)
        Where:
          table_id: is the table id returned by load_table()
        Returns 0 on success
    '''
    ierr = _cmip6_cv.set_institution(table_id)

    return(ierr)


def check_sourceID(table_id):
    '''
      Validate source and source id against Control Vocabulary file.
      Usage:
        cmip6_cv.check_sourceID(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_sourceID(table_id)
    return(ierr)


def check_filename(table_id, var_name, calendar, timeunits, infile):
    '''
      Validate filename with timestamp for current variable and file

      Usage:
        cmip6_cv.check_filename(table_id, var_id)
      Where:
        table_id is the table id returned by load_table()
        var_name is the variable name
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_filename(table_id, var_name,
                                    calendar, timeunits, infile)
    return(ierr)


def check_experiment(table_id):
    '''
      Validate Experiment and Experiement_id against Control Vocabulary file.

      If required global attributes are not all set, warning will be displayed.

      Usage:
        cmip6_cv.check_experiement(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_experiment(table_id)
    return(ierr)


def check_grids(table_id):
    '''
      Validate grid and grid_resolution against Control Vocabulary file.

      Usage:
        cmip6_cv.check_grids(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_grids(table_id)
    return(ierr)


def check_requiredattributes(table_id):
    '''
      Validate all required attributes against Control Vocabulary file.

      Usage:
        cmip6_cv.check_requiredattributes(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_gblattributes(table_id)
    return(ierr)


def check_subExpID(table_id):
    '''
      Validate that sub_experiment ind sub_experiment_id are set to appropriate value
      as defined in the CV file.

      Usage:
        cmip6_cv.check_subExpID(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_subExpID(table_id)
    return(ierr)


def check_parentExpID(table_id):
    '''
      Validate that parent_experiement is set to appropriate value
      if parent is set to "no parent" validate that other related
      attributes are set to "no parent"

      if parent is set to any string, validate all related attributes.

      Usage:
        cmip6_cv.check_parentExpID(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_parentExpID(table_id)
    return(ierr)


def check_ISOTime():
    '''
      Validate that creation attribute contains time
      in ISO format YYYY-MM-DDTHH:MM:SS.

      Usage:
        cmip6_cv.check_ISOtime()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_ISOTime()
    return(ierr)


def setup_variable(name, units, missing, imissing, startime,
                   endtime, startimebnds, endtimebnds):
    '''
    Create variable  attributes from the table loaded by load_table.

    Usage:
        cmip6_cv.check_variable( name, units, missing )
    Where:
        name is the variable name to validate
        units are the variable units
        missing is the missing floating-point value for this variable.
        imissing is the missing integer value for this variable.
        startime: time value for first part of timestap -- time[0]
        endtime: time value for last part of timestap -- time[-1]
        startimebnds: time bound value for first part of timestap -- time_bnds[0]
        endtimebnds: time bound value for last part of timestap -- time_bnds[-1]
    return: variable_id on success
           -1 on failure
    '''
    ierr = _cmip6_cv.setup_variable(
        name,
        units,
        missing,
        imissing,
        startime,
        endtime,
        startimebnds,
        endtimebnds)
    return(ierr)


def reset_CV_Error():
    '''
    set CV_Error to 0
    '''
    _cmip6_cv.reset_CV_Error()
    return


def get_CV_Error():
    '''
    return 0 if no error was set by CV
    '''
    ierr = _cmip6_cv.get_CV_Error()
    return(ierr)


def set_CV_Error():
    '''
    set CV_ERROR to 1.
    '''
    _cmip6_cv.set_CV_Error()
    return


def check_furtherinfourl(table_id):
    '''
      Validate further info URL attribute using REGEX found in Control Vocabulary file.

      Usage:
        cmip6_cv.furtherinfourl(table_id)
      Where:
        table_id is the table id returned by load_table()
      Return 0 on success
    '''
    ierr = _cmip6_cv.check_furtherinfourl(table_id)
    return(ierr)
