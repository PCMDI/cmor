#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Please first complete the following steps:
#
#   1. Download
#      https://github.com/PCMDI/cmip6-cmor-tables.git
#      Create a soft link cmip6-cmor-tables/Tables to ./Tables in your
#      working directory
#
#   python CMIP6Validtor ../Tables/CMIP6_Amon.json ../CMIP6/yourfile.nc
#

"""
Created on Fri Feb 19 11:33:52 2016

@author: Denis Nadeau LLNL
@co-author: Guillaume Levavasseur (IPSL) Parallelization

"""

import argparse
import itertools
import json
import os
import re
import sys
from argparse import ArgumentTypeError
from contextlib import contextmanager
from multiprocessing import Pool
from uuid import uuid4 as uuid

import numpy

# Make sure cdms2.__init__py is not loaded when importing Cdunif
sys.path.insert(0, sys.prefix + "/lib/python2.7/site-packages/cdms2")
import Cdunif
import cmip6_cv


# =========================
# BColors()
# =========================
class BColors:
    """
    Background colors for print statements

    """

    def __init__(self):
        self.HEADER = '\033[95m'
        self.OKBLUE = '\033[94m'
        self.OKGREEN = '\033[1;32m'
        self.WARNING = '\033[1;34;47m'
        self.FAIL = '\033[1;31;47m'
        self.ENDC = '\033[0m'
        self.BOLD = '\033[1m'
        self.UNDERLINE = '\033[4m'


BCOLORS = BColors()


# =========================
# DIRAction()
# =========================
class DIRECTORYAction(argparse.Action):
    """
    Check if argparse is a directory.

    """

    def __call__(self, parser, namespace, values, option_string=None):
        prospective = values
        if not os.path.isdir(prospective):
            if not os.path.isfile(prospective):
                raise argparse.ArgumentTypeError(
                    'No such file/directory: {}'.format(prospective))

        if os.access(prospective, os.R_OK):
            setattr(namespace, self.dest, prospective)
        else:
            raise argparse.ArgumentTypeError(
                'Read access denied: {}'.format(prospective))


# =========================
# INPUTAction()
# =========================
class INPUTAction(argparse.Action):
    """
    Checks if the supplied input exists.

    """

    def __call__(self, parser, namespace, values, option_string=None):
        checked_values = [self.input_checker(x) for x in values]
        setattr(namespace, self.dest, checked_values)

    @staticmethod
    def input_checker(path):
        path = os.path.abspath(os.path.normpath(path))
        if not os.path.exists(path):
            msg = 'No such input: {}'.format(path)
            raise argparse.ArgumentTypeError(msg)
        return path


# =========================
# Collector()
# =========================
class Collector(object):
    """
    Base collector class to yield regular NetCDF files.

    :param list sources: The list of sources to parse
    :returns: The data collector
    :rtype: *iter*

    """

    def __init__(self, sources, data=None):
        self.sources = sources
        self.data = data
        self.FileFilter = FilterCollection()
        self.PathFilter = FilterCollection()
        assert isinstance(self.sources, list)

    def __iter__(self):
        for source in self.sources:
            if os.path.isdir(source):
                # If input is a directory: walk through it and yields netCDF files
                for root, _, filenames in os.walk(source, followlinks=True):
                    if self.PathFilter(root):
                        for filename in sorted(filenames):
                            ffp = os.path.join(root, filename)
                            if os.path.isfile(ffp) and self.FileFilter(filename):
                                yield (ffp, self.data)
            else:
                # It input is a file: yields the netCDF file itself
                root, filename = os.path.split(source)
                if self.PathFilter(root) and self.FileFilter(filename):
                    yield (source, self.data)


class FilterCollection(object):
    """
    Regex dictionary with a call method to evaluate a string against several regular expressions.
    The dictionary values are 2-tuples with the regular expression as a string and a boolean
    indicating to match (i.e., include) or non-match (i.e., exclude) the corresponding expression.

    """
    FILTER_TYPES = (str, re._pattern_type)

    def __init__(self):
        self.filters = dict()

    def add(self, name=None, regex='*', inclusive=True):
        """Add new filter"""
        if not name:
            name = uuid()
        assert isinstance(regex, self.FILTER_TYPES)
        assert isinstance(inclusive, bool)
        self.filters[name] = (regex, inclusive)

    def __call__(self, string):
        return all([self.match(regex, string, inclusive=inclusive) for regex, inclusive in self.filters.values()])

    @staticmethod
    def match(pattern, string, inclusive=True):
        # Assert inclusive and exclusive flag are mutually exclusive
        if inclusive:
            return True if re.search(pattern, string) else False
        else:
            return True if not re.search(pattern, string) else False


# =========================
# Spinner()
# =========================
class Spinner:
    """
    Spinner pending files checking.

    """
    STATES = ('/', '-', '\\', '|')
    step = 0

    def __init__(self):
        self.next()

    def next(self):
        sys.stdout.write('\rChecking data... {}'.format(
            Spinner.STATES[Spinner.step % 4]))
        sys.stdout.flush()
        Spinner.step += 1


# =========================
# checkCMIP6()
# =========================
class checkCMIP6(object):
    """
    Validate if a file is CMIP6 compliant and ready for publication.

    Class need to read CMIP6 Table and Controled Vocabulary file.

    As well,the class will load the EXPERIMENT json file

    Input:
        args.cmip6_table:  CMIP6 table used to creat this file,
                           variable attributes and dimensions will be controled.
        args.CV:           Controled Vocabulary "json" file.

    Output:
        outfile:      Log file, default is stdout.

    """

    def __init__(self, table_path):
        # -------------------------------------------------------------------
        #  Reset CV error switch
        # -------------------------------------------------------------------
        self.cv_error = False
        # -------------------------------------------------------------------
        #  Initialize table path
        # -------------------------------------------------------------------
        self.cmip6_table_path = os.path.normpath(table_path)
        # -------------------------------------------------------------------
        #  Initialize attributes dictionaries
        # -------------------------------------------------------------------
        self.dictGbl = dict()
        self.dictVars = dict()
        # -------------------------------------------------------------------
        # call setup() to clean all 'C' internal memory.
        # -------------------------------------------------------------------
        cmip6_cv.setup(inpath="../Tables", exit_control=cmip6_cv.CMOR_NORMAL)
        # -------------------------------------------------------------------
        # Set Control Vocabulary file to use (default from cmor.h)
        # -------------------------------------------------------------------
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.GLOBAL_CV_FILENAME,
            cmip6_cv.TABLE_CONTROL_FILENAME)
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.FILE_PATH_TEMPLATE,
            cmip6_cv.CMOR_DEFAULT_PATH_TEMPLATE)
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.FILE_NAME_TEMPLATE,
            cmip6_cv.CMOR_DEFAULT_FILE_TEMPLATE)
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.GLOBAL_ATT_FURTHERINFOURLTMPL,
            cmip6_cv.CMOR_DEFAULT_FURTHERURL_TEMPLATE)
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.CMOR_AXIS_ENTRY_FILE,
            "CMIP6_coordinate.json")
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.CMOR_FORMULA_VAR_FILE,
            "CMIP6_formula_terms.json")

    @staticmethod
    def _get_variable_from_filename(f):
        return f.split('_')[0]

    @staticmethod
    def _get_table_from_filename(f):
        return f.split('_')[1]

    @staticmethod
    def _check_json_table(path):
        f = open(path)
        lines = f.readlines()
        f.close()
        jsonobject = json.loads(" ".join(lines))
        if not jsonobject:
            raise argparse.ArgumentTypeError(
                'Invalid JSON CMOR table: {}'.format(path))

    def set_double_value(self, attribute):
        if cmip6_cv.has_cur_dataset_attribute(attribute):
            if isinstance(self.dictGbl[attribute], numpy.ndarray) and \
                    isinstance(self.dictGbl[attribute][0], numpy.float64):
                self.dictGbl[attribute] = self.dictGbl[attribute][0]
                cmip6_cv.set_cur_dataset_attribute(attribute, self.dictGbl[attribute])

    def ControlVocab(self, ncfile, variable=None):
        """
        Check CMIP6 global attributes against Control Vocabulary file.

            1. Validate required attribute if presents and some values.
            2. Validate registered institution and institution_id
            3. Validate registered source and source_id
            4. Validate experiment, experiment_id and all attributes associated with this experiment.
                   Make sure that all attributes associate with the experiment_id found in CMIP6_CV.json
                   are set to the appropriate values.
            5. Validate grid_label and grid_resolution
            6. Validate creation time in ISO format (YYYY-MM-DDTHH:MM:SS)
            7. Validate furtherinfourl from CV internal template
            8. Validate variable attributes with CMOR JSON table.
            9. Validate parent_* attribute
           10. Validate sub_experiment_* attributes.
           11. Validate that all *_index are integers.

        """
        filename = os.path.basename(ncfile)
        # -------------------------------------------------------------------
        #  Initialize arrays
        # -------------------------------------------------------------------
        # If table_path is the table directory
        # Deduce corresponding JSON from filename
        if os.path.isdir(self.cmip6_table_path):
            cmip6_table = '{}/CMIP6_{}.json'.format(
                self.cmip6_table_path, self._get_table_from_filename(filename))
        else:
            cmip6_table = self.cmip6_table_path
        # Check JSON file
        self._check_json_table(cmip6_table)
        # -------------------------------------------------------------------
        # Load CMIP6 table into memory
        # -------------------------------------------------------------------
        table_id = cmip6_cv.load_table(cmip6_table)
        # -------------------------------------------------------------------
        #  Deduce variable
        # -------------------------------------------------------------------
        # If variable can be deduced from the filename (Default)
        if not variable:
            variable = self._get_variable_from_filename(filename)
        else:
            # If variable submitted on command line with --variable
            variable = variable
        # -------------------------------------------------------------------
        #  Is a climatology?
        # -------------------------------------------------------------------
        climatology = False
        if filename.find('-clim') != -1:
            climatology = True
            if cmip6_table.find('Amon') != -1:
                variable = '{}Clim'.format(variable)
        # -------------------------------------------------------------------
        #  Open file in processing
        # -------------------------------------------------------------------
        infile = Cdunif.CdunifFile(ncfile, "r")
        # --------------------------------------
        # Create a list of all Global Attributes
        # --------------------------------------
        self.dictGbl = infile.__dict__
        for key, value in self.dictGbl.iteritems():
            cmip6_cv.set_cur_dataset_attribute(key, value)
        # Set member_id attribute depending on sub_experiment_id and variant_label
        member_id = ""
        if "sub_experiment_id" in self.dictGbl.keys():
            if self.dictGbl["sub_experiment_id"] not in ['none']:
                member_id = '{}-{}'.format(self.dictGbl['sub_experiment_id'],
                                           self.dictGbl['variant_label'])
            else:
                member_id = self.dictGbl['variant_label']
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.GLOBAL_ATT_MEMBER_ID, member_id)
        self.set_double_value('branch_time_in_parent')
        self.set_double_value('branch_time_in_child')
        if variable is not None:
            var = [variable]
        else:
            # -------------------------------------------------------------------
            # find variable that contains a "history" (should only be one)
            # -------------------------------------------------------------------
            var = [infile.variable_id]

        clim_idx = var[0].find('Clim')
        if climatology and clim_idx != -1:
            var = [var[0][:clim_idx]]

        if var == [] or len(var) > 1:
            print BCOLORS.FAIL
            print "====================================================================================="
            print "The input file does not have an history attribute and the CMIP6 variable could not be found"
            print "Please use the --variable option to specify your CMIP6 variable"
            print "====================================================================================="
            print BCOLORS.ENDC
            raise KeyboardInterrupt

        try:
            var_keys = infile.variables[var[0]].__dict__.keys()
        except BaseException:
            print BCOLORS.FAIL
            print "====================================================================================="
            print "The variable " + var[0] + " could not be found"
            print "====================================================================================="
            print BCOLORS.ENDC
            raise KeyboardInterrupt

        # -------------------------------------------------------------------
        # Create a dictionary of attributes for var
        # -------------------------------------------------------------------
        self.dictVars = dict((y, x) for y, x in
                             [(key, value) for key in var_keys
                              if infile.variables[var[0]].__dict__[key] is not None
                              for value in [infile.variables[var[0]].__dict__[key]]])
        try:
            calendar = infile.variables['time'].calendar
            timeunits = infile.variables['time'].units
        except BaseException:
            calendar = "gregorian"
            timeunits = "days since ?"
        cmip6_cv.check_requiredattributes(table_id)
        cmip6_cv.check_institution(table_id)
        cmip6_cv.check_sourceID(table_id)
        cmip6_cv.check_experiment(table_id)
        cmip6_cv.check_grids(table_id)
        cmip6_cv.check_ISOTime()
        cmip6_cv.check_furtherinfourl(table_id)
        cmip6_cv.check_parentExpID(table_id)
        cmip6_cv.check_subExpID(table_id)

        try:
            if climatology:
                startimebnds = infile.variables['climatology_bnds'][0][0]
                endtimebnds = infile.variables['climatology_bnds'][-1][1]
            else:
                startimebnds = infile.variables['time_bnds'][0][0]
                endtimebnds = infile.variables['time_bnds'][-1][1]
        except BaseException:
            startimebnds = 0
            endtimebnds = 0

        try:
            startime = infile.variables['time'][0]
            endtime = infile.variables['time'][-1]
        except BaseException:
            startime = 0
            endtime = 0

        varunits = infile.variables[var[0]].units
        varmissing = infile.variables[var[0]]._FillValue[0]
        # -------------------------------------------------
        # Make sure with use variable for Climatology
        # -------------------------------------------------
        varid = cmip6_cv.setup_variable(variable, varunits, varmissing, startime, endtime,
                                        startimebnds, endtimebnds)
        if varid == -1:
            print BCOLORS.FAIL
            print "====================================================================================="
            print "Could not find variable '%s' in table '%s' " % (var[0], cmip6_table)
            print "====================================================================================="
            print BCOLORS.ENDC
            self.cv_error = True
            raise KeyboardInterrupt

        fn = os.path.basename(str(infile).split('\'')[1])
        cmip6_cv.check_filename(
            table_id,
            varid,
            calendar,
            timeunits,
            fn)

        if 'branch_time_in_child' in self.dictGbl.keys():
            if not isinstance(self.dictGbl['branch_time_in_child'], numpy.float64):
                print BCOLORS.FAIL
                print "====================================================================================="
                print "branch_time_in_child is not a double: ", type(self.dictGbl['branch_time_in_child'])
                print "====================================================================================="
                print BCOLORS.ENDC
                self.cv_error = True

        if 'branch_time_in_parent' in self.dictGbl.keys():
            if not isinstance(self.dictGbl['branch_time_in_parent'], numpy.float64):
                print BCOLORS.FAIL
                print "====================================================================================="
                print "branch_time_in_parent is not an double: ", type(self.dictGbl['branch_time_in_parent'])
                print "====================================================================================="
                print BCOLORS.ENDC
                self.cv_error = True

        if not isinstance(self.dictGbl['realization_index'], numpy.ndarray):
            print BCOLORS.FAIL
            print "====================================================================================="
            print "realization_index is not an integer: ", type(self.dictGbl['realization_index'])
            print "====================================================================================="
            print BCOLORS.ENDC
            self.cv_error = True

        if not isinstance(self.dictGbl['initialization_index'], numpy.ndarray):
            print BCOLORS.FAIL
            print "====================================================================================="
            print "initialization_index is not an integer: ", type(self.dictGbl['initialization_index'])
            print "====================================================================================="
            print BCOLORS.ENDC
            self.cv_error = True

        if not isinstance(self.dictGbl['physics_index'], numpy.ndarray):
            print BCOLORS.FAIL
            print "====================================================================================="
            print "physics_index is not an integer: ", type(self.dictGbl['physics_index'])
            print "====================================================================================="
            print BCOLORS.ENDC
            self.cv_error = True

        if not isinstance(self.dictGbl['forcing_index'], numpy.ndarray):
            print BCOLORS.FAIL
            print "====================================================================================="
            print "forcing_index is not an integer: ", type(self.dictGbl['forcing_index'])
            print "====================================================================================="
            print BCOLORS.ENDC
            self.cv_error = True

        # -----------------------------
        # variable attribute comparison
        # -----------------------------
        cv_attrs = cmip6_cv.list_variable_attributes(varid)
        for key in cv_attrs:
            if key == "long_name":
                continue
            if key == "comment":
                continue
            if key == "cell_measures":
                if cv_attrs[key].find("OPT") != -1 or cv_attrs[key].find("MODEL") != -1:
                    continue
            # Is this attribute in file?
            if key in self.dictVars.keys():
                # Verify that attribute value is equal to file attribute
                table_value = cv_attrs[key]
                file_value = self.dictVars[key]

                # PrePARE accept units of 1 or 1.0 so adjust thet table_value
                # -----------------------------------------------------------
                if key == "units":
                    if (table_value == "1") and (file_value == "1.0"):
                        table_value = "1.0"
                    if (table_value == "1.0") and (file_value == "1"):
                        table_value = "1"

                if isinstance(table_value, str) and isinstance(file_value, numpy.ndarray):
                    if numpy.array([int(value) for value in table_value.split()] == file_value).all():
                        file_value = True
                        table_value = True

                if isinstance(table_value, numpy.ndarray):
                    table_value = table_value[0]
                if isinstance(file_value, numpy.ndarray):
                    file_value = file_value[0]
                if isinstance(table_value, float):
                    if file_value == 0:
                        if table_value != file_value:
                            file_value = False
                    else:
                        if 1 - (table_value / file_value) < 0.00001:
                            table_value = file_value

                if key == "cell_methods":
                    idx = file_value.find(" (")
                    if idx != -1:
                        file_value = file_value[:idx]
                        table_value = table_value[:idx]

                if key == "cell_measures":
                    pattern = re.compile('(?P<param>[\w.-]+): (?P<val1>[\w.-]+) OR (?P<val2>[\w.-]+)')
                    values = re.findall(pattern, table_value)
                    table_values = [""]  # Empty string is allowed in case of useless attribute
                    if values:
                        tmp = dict()
                        for param, val1, val2 in values:
                            tmp[param] = [str('{}: {}'.format(param, val1)), str('{}: {}'.format(param, val2))]
                        table_values.extend([' '.join(i) for i in list(itertools.product(*tmp.values()))])
                        if str(file_value) not in map(str, table_values):
                            print BCOLORS.FAIL
                            print "====================================================================================="
                            print "Your file contains \"" + key + "\":\"" + str(file_value) + "\" and"
                            print "CMIP6 tables requires \"" + key + "\":\"" + str(table_value) + "\"."
                            print "====================================================================================="
                            print BCOLORS.ENDC
                            self.cv_error = True
                        continue

                if str(table_value) != str(file_value):
                    print BCOLORS.FAIL
                    print "====================================================================================="
                    print "Your file contains \"" + key + "\":\"" + str(file_value) + "\" and"
                    print "CMIP6 tables requires \"" + key + "\":\"" + str(table_value) + "\"."
                    print "====================================================================================="
                    print BCOLORS.ENDC
                    self.cv_error = True
            else:
                # That attribute is not in the file
                table_value = cv_attrs[key]
                if isinstance(table_value, numpy.ndarray):
                    table_value = table_value[0]
                if isinstance(table_value, float):
                    table_value = "{0:.2g}".format(table_value)
                print BCOLORS.FAIL
                print "====================================================================================="
                print "CMIP6 variable " + var[0] + " requires \"" + key + "\":\"" + str(table_value) + "\"."
                print "====================================================================================="
                print BCOLORS.ENDC
                self.cv_error = True

        if self.cv_error or (cmip6_cv.get_CV_Error()):
            raise KeyboardInterrupt
        else:
            print BCOLORS.OKGREEN
            print "*************************************************************************************"
            print "* This file is compliant with the CMIP6 specification and can be published in ESGF  *"
            print "*************************************************************************************"
            print BCOLORS.ENDC


def process(source):
    # Redirect all print statements to a logfile dedicated to the current
    # process
    logfile = '/tmp/PrePARE-{}.log'.format(os.getpid())
    with RedirectedOutput(logfile):
        rc = sequential_process(source)
    # Close and return logfile
    return (logfile, rc)


def sequential_process(source):
    try:
        # Deserialize inputs
        ncfile, data = source
        table_path, variable = data
        print "Processing: {}\n".format(ncfile)
        # Process file
        checker = checkCMIP6(table_path)
        if variable:
            checker.ControlVocab(ncfile, variable)
        else:
            checker.ControlVocab(ncfile)
        rc = 0
    except KeyboardInterrupt:
        rc = 1
        print BCOLORS.FAIL
        print "*************************************************************************************"
        print "* Error: The input file is not CMIP6 compliant                                      *"
        print "* Check your file or use CMOR 3.x to achieve compliance for ESGF publication        *"
        print "*************************************************************************************"
        print BCOLORS.ENDC
    finally:
        # Close opened file
        if hasattr(checker, "infile"):
            checker.infile.close()
        return(rc)


def regex_validator(string):
    """
    Validates a Python regular expression syntax.

    :param str string: The string to check
    :returns: The Python regex
    :rtype: *re.compile*
    :raises Error: If invalid regular expression

    """
    try:
        return re.compile(string)
    except re.error:
        msg = 'Bad regex syntax: {}'.format(string)
        raise ArgumentTypeError(msg)


#  =========================
#   main()
#  =========================
def main():
    parser = argparse.ArgumentParser(
        prog='PrePARE',
        description='Validate CMIP6 file for ESGF publication.')

    parser.add_argument(
        '--variable',
        help='Specify geophysical variable name.\n'
             'If not variable is deduced from filename.')

    parser.add_argument(
        '--table-path',
        help='Specify the CMIP6 CMOR tables path (JSON file).\n'
             'If a directory is submitted table is deduced from filename (default is "./Tables").',
        action=DIRECTORYAction,
        default='./Tables')

    parser.add_argument(
        '--max-threads',
        type=int,
        default=1,
        help='Number of maximal threads to simultaneously process several files.\n'
             'Default is one as sequential processing.')

    parser.add_argument(
        '--ignore-dir',
        metavar="PYTHON_REGEX",
        type=str,
        default='^.*/\.[\w]*$',
        help='Filter directories NON-matching the regular expression.\n'
             'Default ignores paths with folder name(s) starting with "."')

    parser.add_argument(
        '--include-file',
        metavar='PYTHON_REGEX',
        type=regex_validator,
        action='append',
        help='Filter files matching the regular expression.\n'
             'Duplicate the flag to set several filters.\n'
             'Default only include NetCDF files.')

    parser.add_argument(
        '--exclude-file',
        metavar='PYTHON_REGEX',
        type=regex_validator,
        action='append',
        help='Filter files NON-matching the regular expression.\n'
             'Duplicate the flag to set several filters.\n'
             'Default only exclude hidden files (with names not\n'
             'starting with ".").')

    parser.add_argument(
        'input',
        help='Input CMIP6 netCDF data to validate (ex: clisccp_cfMon_DcppC22_NICAM_gn_200001-200001.nc).\n'
             'If a directory is submitted all netCDF recursively found will be validate independently.',
        nargs='+',
        action=INPUTAction)

    # Check command-line error
    try:
        args = parser.parse_args()
    except argparse.ArgumentTypeError as errmsg:
        print >> sys.stderr, str(errmsg)
        return 1
    except SystemExit:
        return 1

    # Collects netCDF files for process
    sources = Collector(args.input, data=(args.table_path, args.variable))
    # Set scan filters
    file_filters = list()
    if args.include_file:
        file_filters.extend([(f, True) for f in args.include_file])
    else:
        # Default includes netCDF only
        file_filters.append(('^.*\.nc$', True))
    if args.exclude_file:
        # Default exclude hidden files
        file_filters.extend([(f, False) for f in args.exclude_file])
    else:
        file_filters.append(('^\..*$', False))
    # Init collector file filter
    for regex, inclusive in file_filters:
        sources.FileFilter.add(regex=regex, inclusive=inclusive)
    # Init collector dir filter
    sources.PathFilter.add(regex=args.ignore_dir, inclusive=False)
    returnCode = 0
    # Separate sequential process and multiprocessing
    if args.max_threads > 1:
        # Create pool of processes
        pool = Pool(int(args.max_threads))
        # Run processes
        logfiles = list()
        progress = Spinner()
        for logfile in pool.imap(process, sources):
            progress.next()
            logfiles.append(logfile)
        sys.stdout.write('\r\033[K')
        sys.stdout.flush()
        # Print results from logfiles and remove them
        for (logfile, rc) in set(logfiles):
            with open(logfile, 'r') as f:
                print f.read()
            os.remove(logfile)
            returnCode = returnCode + rc
        # Close pool of processes
        pool.close()
        pool.join()
    else:
        for source in sources:
            returnCode = sequential_process(source)

    return returnCode


@contextmanager
def RedirectedOutput(to=os.devnull):
    fd_out = sys.stdout.fileno()
    old_stdout = os.fdopen(os.dup(fd_out), 'w')
    fd_err = sys.stderr.fileno()
    old_stderr = os.fdopen(os.dup(fd_err), 'w')
    stream = open(to, 'a+')
    sys.stdout.close()
    sys.stderr.close()
    os.dup2(stream.fileno(), fd_out)
    os.dup2(stream.fileno(), fd_err)
    sys.stdout = os.fdopen(fd_out, 'w')
    sys.stderr = os.fdopen(fd_err, 'w')
    try:
        yield
    finally:
        sys.stdout.close()
        sys.stderr.close()
        os.dup2(old_stdout.fileno(), fd_out)
        os.dup2(old_stderr.fileno(), fd_err)
        sys.stdout = os.fdopen(fd_out, 'w')
        sys.stderr = os.fdopen(fd_err, 'w')


if __name__ == '__main__':
    sys.exit(main())
