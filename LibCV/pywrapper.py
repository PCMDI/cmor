
import cmor_const,numpy,os,_cmip6_cv
import signal

def sig_handler(signum, frame):
    os.kill(os.getpid(),signal.SIGABRT)

signal.signal(signal.SIGTERM, sig_handler)

try:
    import cdtime
    has_cdtime = True
except:
    has_cdtime = False

try:
    import cdms2
    has_cdms2 = True
except:
    has_cdms2 = False

try:
    import MV2
    has_MV2 = True
except:
    has_MV2 = False

try:
    import numpy.oldnumeric.ma.MaskedArray
    has_oldma = True
except:
    has_oldma = False

def _to_numpy(vals, message):
    if isinstance(vals, (list, tuple)):
        vals = numpy.ascontiguousarray(vals)
    elif not isinstance(vals, numpy.ndarray):
        try:
            vals = numpy.ascontiguousarray(vals.filled())
        except:
            raise Exception, "Error could not convert %s to a numpy array" % message

    return vals


def setup(inpath='.',netcdf_file_action=cmor_const.CMOR_PRESERVE,set_verbosity=cmor_const.CMOR_NORMAL,exit_control=cmor_const.CMOR_NORMAL,logfile=None,create_subdirectories=1):
    """
    Usage cmor_setup(inpath='.',netcdf_file_action=cmor.CMOR_PRESERVE,set_verbosity=cmor.CMOR_NORMAL,exit_control=cmor.CMOR_NORMAL)
    Where:
    path:                  Alternate directory where to find tables if not in current directory
    netcdf_file_action:    What to do when opening the netcdf file, valid options are:
                           CMOR_PRESERVE, CMOR_APPEND, CMOR_REPLACE, CMOR_PRESERVE_4, CMOR_APPEND_4, CMOR_REPLACE_4, CMOR_PRESERVE_3, CMOR_APPEND_3 or CMOR_REPLACE_3
                           The _3 means netcdf will be created in the old NetCDF3 format (no compression nor chunking), _4 means use NetCDF4 classic format. No _ is equivalent to _3

    set_verbosity:         CMOR_QUIET or CMOR_NORMAL
    exit_control:          CMOR_EXIT_ON_WARNING, CMOR_EXIT_ON_MAJOR, CMOR_NORMAL
    create_subdirectories: 1 to create subdirectories structure, 0 to dump files directly where cmor_dataset tells to
"""
    if not isinstance(exit_control,int) or not exit_control in [ cmor_const.CMOR_EXIT_ON_WARNING, cmor_const.CMOR_EXIT_ON_MAJOR, cmor_const.CMOR_NORMAL]:
        raise Exception, "exit_control must an integer valid values are: CMOR_EXIT_ON_WARNING, CMOR_EXIT_ON_MAJOR, CMOR_NORMAL"

    if not isinstance(netcdf_file_action,int) or not netcdf_file_action in [ cmor_const.CMOR_PRESERVE, cmor_const.CMOR_APPEND, cmor_const.CMOR_REPLACE, cmor_const.CMOR_PRESERVE_3, cmor_const.CMOR_APPEND_3, cmor_const.CMOR_REPLACE_3,cmor_const.CMOR_PRESERVE_4, cmor_const.CMOR_APPEND_4, cmor_const.CMOR_REPLACE_4 ]:
        raise Exception, "netcdf_file_action must be an integer. Valid values are: CMOR_PRESERVE, CMOR_APPEND, CMOR_REPLACE, CMOR_PRESERVE_3, CMOR_APPEND_3 or CMOR_REPLACE_3, CMOR_PRESERVE_4, CMOR_APPEND_4 or CMOR_REPLACE_4"

    if not isinstance(set_verbosity,int) or not set_verbosity in [ cmor_const.CMOR_QUIET, cmor_const.CMOR_NORMAL]:
        raise Exception, "set_verbosity must an integer valid values are: CMOR_QUIET, CMOR_NORMAL"

    if not isinstance(inpath,str) and not os.path.exists(inpath):
        raise Exception, "path must be a Valid path"
    if logfile is None:
        logfile = ""

    if not create_subdirectories in [0,1]:
        raise Exception, "create_subdirectories must be 0 or 1"
    return _cmip6_cv.setup(inpath,netcdf_file_action,set_verbosity,exit_control,logfile,create_subdirectories)

def load_table(table):
    """ loads a cmor table
    Usage:
    load_table(table)
    """
    if not isinstance(table,str):
        raise Exception, "Error, must pass a string"
##     if not os.path.exists(table):
##         raise Exception, "Error, the table you specified (%s) does not exists" % table
    return _cmip6_cv.load_table(table)

def dataset_json(rcfile):
    """ load dataset JSON file
    Usage:
    dataset_json(rcfile)
    """
    if not isinstance(rcfile,str):
        raise Exception, "Error, must pass a string"
##     if not os.path.exists(table):
##         raise Exception, "Error, the table you specified (%s) does not exists" % table
    return _cmip6_cv.dataset_json(rcfile)

def set_table(table):
    if not isinstance(table,int):
        raise Exception, "error you need to pass and integer as the table id"
    return _cmip6_cv.set_table(table)

def close(var_id=None,file_name=False, preserve=False):
    """ Close CMOR variables/file
    Usage:
      cmor.close(varid=None)
    Where:
      var_id: id of variable to close, if passing None, means close every open ones.
      [file_name] True/False (default False) if True: return name of the file just closed, works only if var_id is not None
      [preserve] True/False (default False) if True: close the file but preserve the var definition in CMOR to write more data with this variable (into a new file)
      """
    if var_id is not None and not isinstance(var_id,int):
        raise Exception, "Error var_id must be None or a integer"

    if (preserve is False):
        if (file_name is False):
            return _cmip6_cv.close(var_id,0,0)
        else:
            return _cmip6_cv.close(var_id,1,0)
    else:
        if (file_name is False):
            return _cmip6_cv.close(var_id,0,1)
        else:
            return _cmip6_cv.close(var_id,1,1)


def set_cur_dataset_attribute(name,value):
    """Sets an attribute onto the current cmor dataset
    Usage:
      cmor.set_cur_dataset_attribute(name,value)
    Where:
      name: is the name of the attribute
      value: is the value for this attribute
    """
    if value is None:
        val=""
    else:
        val = str(value)
    return _cmip6_cv.set_cur_dataset_attribute(name,val)

def has_cur_dataset_attribute(name):
    """determines if the current cmor dataset has an attribute
    Usage:
      cmor.het_cur_dataset_attribute(name)
    Where:
      name: is the name of the attribute
    Returns True if the dataset has the attribute, False otherwise
    """
    test = _cmip6_cv.has_cur_dataset_attribute(name)
    if test == 0 :
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

def set_variable_attribute(var_id,name,value):
    """Sets an attribute onto a cmor variable
    Usage:
      cmor.set_variable_attribute(var_id,name,value)
    Where:
      var_id: is cmor variable id
      name  : is the name of the attribute
      value : is the value for this attribute
    """
    if value is None:
        val=""
    else:
        val = str(value)
    return _cmip6_cv.set_variable_attribute(var_id,name,val)


def has_variable_attribute(var_id,name):
    """determines if the a cmor variable has an attribute
    Usage:
      cmor.het_variable_attribute(name)
    Where:
      var_id: is cmor variable id
      name: is the name of the attribute
    Returns True if the dataset has the attribute, False otherwise
    """
    test = _cmip6_cv.has_variable_attribute(var_id,name)
    if test == 0 :
        return True
    else:
        return False

def get_variable_attribute(var_id,name):
    """Gets an attribute from a cmor variable
    Usage:
      cmor.get_variable_attribute(name)
    Where:
      var_id: is cmor variable id
      name: is the name of the attribute
    Returns none if attribute is non-existant
    """
    ## print 'In there asking for attribute: ',name,'on var',var_id
    if has_variable_attribute(var_id,name):
        ## print 'Seems to have it',var_id,name
        return _cmip6_cv.get_variable_attribute(var_id,name)
    else:
        return None

