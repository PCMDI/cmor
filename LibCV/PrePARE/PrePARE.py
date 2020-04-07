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

from __future__ import print_function
import argparse
import itertools
import json
import os
import re
import sys
from argparse import ArgumentTypeError
from contextlib import contextmanager
from datetime import datetime
from multiprocessing import Pool
from uuid import uuid4 as uuid

import numpy
import netCDF4

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

    def __init__(self, sources):
        self.sources = sources
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
                                yield ffp
            else:
                # It input is a file: yields the netCDF file itself
                root, filename = os.path.split(source)
                if self.PathFilter(root) and self.FileFilter(filename):
                    yield source

    def __len__(self):
        """
        Returns collector length.

        :returns: The number of items in the collector.
        :rtype: *int*

        """
        s = 0
        for _ in self.__iter__():
            s += 1
        return s


class FilterCollection(object):
    """
    Regex dictionary with a call method to evaluate a string against several regular expressions.
    The dictionary values are 2-tuples with the regular expression as a string and a boolean
    indicating to match (i.e., include) or non-match (i.e., exclude) the corresponding expression.

    """
    if sys.version_info < (3, 7):
        FILTER_TYPES = (str, re._pattern_type)
    else:
        FILTER_TYPES = (str, re.Pattern)

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
        return all([self.match(regex, string, inclusive=inclusive) for regex, inclusive in list(self.filters.values())])

    @staticmethod
    def match(pattern, string, inclusive=True):
        # Assert inclusive and exclusive flag are mutually exclusive
        if inclusive:
            return True if re.search(pattern, string) else False
        else:
            return True if not re.search(pattern, string) else False


# =========================
# ProcessContext()
# =========================
class ProcessContext(object):
    """
    Encapsulates the processing context/information for child process.

    :param dict args: Dictionary of argument to pass to child process
    :returns: The processing context
    :rtype: *ProcessContext*

    """

    def __init__(self, args):
        assert isinstance(args, dict)
        for key, value in list(args.items()):
            setattr(self, key, value)


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
        self.errors = 0
        # -------------------------------------------------------------------
        #  Initialize table path
        # -------------------------------------------------------------------
        self.cmip6_table_path = os.path.normpath(table_path)
        # -------------------------------------------------------------------
        #  Initialize attributes dictionaries
        # -------------------------------------------------------------------
        self.dictGbl = dict()
        self.dictVar = dict()
        # -------------------------------------------------------------------
        # call setup() to clean all 'C' internal memory.
        # -------------------------------------------------------------------
        cmip6_cv.setup(inpath="../Tables", exit_control=cmip6_cv.CMOR_EXIT_ON_WARNING)
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

    def prepare_print(self, msg, code, no_text_color=False, lines=False):
        code_color = {
            'HEADER': BCOLORS.HEADER,
            'OKBLUE': BCOLORS.OKBLUE,
            'OKGREEN': BCOLORS.OKGREEN,
            'WARNING': BCOLORS.WARNING,
            'FAIL': BCOLORS.FAIL,
            'BOLD': BCOLORS.BOLD,
            'UNDERLINE': BCOLORS.UNDERLINE,
        }
        line = '====================================================================================='
        if lines:
            if code in code_color and not no_text_color:
                print(code_color[code])
            else:
                print('')
            print(line)
            print(msg)
            print(line)
            if code in code_color and not no_text_color:
                print(BCOLORS.ENDC)
            else:
                print('')
        else:
            if no_text_color:
                print(msg)
            elif code in code_color:
                print('{}{}{}'.format(code_color[code], msg, BCOLORS.ENDC))
            else:
                print(msg)

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
        return jsonobject

    def set_double_value(self, attribute):
        if cmip6_cv.has_cur_dataset_attribute(attribute):
            if isinstance(self.dictGbl[attribute], numpy.ndarray) and \
                    isinstance(self.dictGbl[attribute][0], numpy.float64):
                self.dictGbl[attribute] = self.dictGbl[attribute][0]
                cmip6_cv.set_cur_dataset_attribute(attribute, self.dictGbl[attribute])

    @staticmethod
    def is_climatology(filename, **kwargs):
        return True if filename.find('-clim') != -1 else False

    @staticmethod
    def has_3_dimensions(infile, variable, **kwargs):
        return True if len(infile.variables[variable].dimensions) == 3 else False

    @staticmethod
    def has_27_pressure_levels(infile, **kwargs):
        dim = [d for d in list(infile.dimensions.keys()) if 'plev' in d]
        return True if len(dim) == 1 and infile.dimensions[dim[0]].size == 27 else False

    @staticmethod
    def has_7_pressure_levels(infile, **kwargs):
        dim = [d for d in list(infile.dimensions.keys()) if 'plev' in d]
        return True if len(dim) == 1 and infile.dimensions[dim[0]].size == 7 else False

    @staticmethod
    def has_4_pressure_levels(infile, **kwargs):
        dim = [d for d in list(infile.dimensions.keys()) if 'plev' in d]
        return True if len(dim) == 1 and infile.dimensions[dim[0]].size == 4 else False

    @staticmethod
    def has_land_in_cell_methods(infile, variable, **kwargs):
        return True if 'land' in infile.variables[variable].cell_methods else False

    @staticmethod
    def has_variable_name(filename, **kwargs):
        return True

    def ControlVocab(self, ncfile, variable=None, print_all=True, no_text_color=False):
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
        table_id = os.path.basename(os.path.splitext(cmip6_table)[0]).split('_')[1]
        # Check and get JSON table
        cmor_table = self._check_json_table(cmip6_table)
        # -------------------------------------------------------------------
        # Load CMIP6 table into memory
        # -------------------------------------------------------------------
        table = cmip6_cv.load_table(cmip6_table)
        # -------------------------------------------------------------------
        #  Deduce variable
        # -------------------------------------------------------------------
        # If variable can be deduced from the filename (Default)
        # If not variable submitted on command line with --variable is considered
        variable_id = self._get_variable_from_filename(filename)
        if not variable:
            variable = variable_id
        # -------------------------------------------------------------------
        #  Distinguish similar CMOR entries with the same out_name if exist
        # -------------------------------------------------------------------
        # Apply test on variable only if a particular treatment if required
        prepare_path = os.path.dirname(os.path.realpath(__file__))
        out_names_tests = json.loads(open(os.path.join(prepare_path, 'out_names_tests.json')).read())
        # -------------------------------------------------------------------
        #  Open file in processing
        #  The file needs to be open before the calling the test.
        # -------------------------------------------------------------------
        infile = netCDF4.Dataset(ncfile, "r")
        key = '{}_{}'.format(table_id, variable_id)
        variable_cmor_entry = None
        if key in list(out_names_tests.keys()):
            for test, cmor_entry in list(out_names_tests[key].items()):
                if getattr(self, test)(**{'infile': infile,
                                          'variable': variable,
                                          'filename': filename}):
                    # If test successfull, the CMOR entry to consider is given by the test
                    variable_cmor_entry = cmor_entry
                    break
                else:
                    # If not, CMOR entry to consider is the variable from filename or from input command-line
                    variable_cmor_entry = variable
        else:
            # By default, CMOR entry to consider is the variable from filename or from input command-line
            variable_cmor_entry = variable
        # -------------------------------------------------------------------
        #  Get variable out name in netCDF record
        #  -------------------------------------------------------------------
        # Variable record name should follow CMOR table out names
        if variable_cmor_entry not in list(cmor_table['variable_entry'].keys()):
            msg = "The entry " + variable_cmor_entry + " could not be found in CMOR table"
            self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
            self.prepare_print("└──> :: CV FAIL    :: {}".format(ncfile), 'FAIL', no_text_color)
            raise KeyboardInterrupt
        variable_record_name = cmor_table['variable_entry'][variable_cmor_entry]['out_name']
        # Variable id attribute should be the same as variable record name
        # in any case to be CF- and CMIP6-compliant
        variable_id = variable_record_name
        # -------------------------------------------------------------------
        # Create a dictionary of all global attributes
        # -------------------------------------------------------------------
        self.dictGbl = infile.__dict__
        for key, value in list(self.dictGbl.items()):
            cmip6_cv.set_cur_dataset_attribute(key, value)
        # Set member_id attribute depending on sub_experiment_id and variant_label
        member_id = ""
        if "sub_experiment_id" in list(self.dictGbl.keys()):
            if self.dictGbl["sub_experiment_id"] not in ['none']:
                member_id = '{}-{}'.format(self.dictGbl['sub_experiment_id'],
                                           self.dictGbl['variant_label'])
            else:
                member_id = self.dictGbl['variant_label']
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.GLOBAL_ATT_MEMBER_ID, member_id)
        # -------------------------------------------------------------------
        # Create a dictionary of attributes for the variable
        # -------------------------------------------------------------------
        try:
            self.dictVar = infile.variables[variable_record_name].__dict__
        except BaseException:
            msg = "The variable " + variable_record_name + " could not be found in file"
            self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
            self.prepare_print("└──> :: CV FAIL    :: {}".format(ncfile), 'FAIL', no_text_color)
            raise KeyboardInterrupt

        # -------------------------------------------------------------------
        # Check global attributes
        # -------------------------------------------------------------------
        if cmip6_cv.check_requiredattributes(table) != 0:
            self.errors += 1
        if cmip6_cv.check_institution(table) != 0:
            self.errors += 1
        if cmip6_cv.check_sourceID(table) != 0:
            self.errors += 1
        if cmip6_cv.check_experiment(table) != 0:
            self.errors += 1
        if cmip6_cv.check_grids(table) != 0:
            self.errors += 1
        if cmip6_cv.check_ISOTime() != 0:
            self.errors += 1
        if cmip6_cv.check_furtherinfourl(table) != 0:
            self.errors += 1
        if cmip6_cv.check_subExpID(table) != 0:
            self.errors += 1
        for attr in ['branch_time_in_child', 'branch_time_in_parent']:
            if attr in list(self.dictGbl.keys()):
                self.set_double_value(attr)
                if not numpy.issubdtype(self.dictGbl[attr], numpy.float64):
                    msg = "{} is not a double: {}".format(attr, type(self.dictGbl[attr]))
                    self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                    self.errors += 1
        for attr in ['realization_index', 'initialization_index', 'physics_index', 'forcing_index']:
            try:
                if not numpy.issubdtype(self.dictGbl[attr], numpy.integer):
                    msg = "{} is not an integer: {}".format(attr, type(self.dictGbl[attr]))
                    self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                    self.errors += 1
            except KeyError:
                msg = "{} attribute is missing in global attributes".format(attr)
                self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                self.errors += 1
        if cmip6_cv.check_parentExpID(table) != 0:
            self.errors += 1
        for attr in ['table_id', 'variable_id']:
            try:
                if locals()[attr] != self.dictGbl[attr]:
                    msg = "{} attribute is not consistent: {}".format(attr, self.dictGbl[attr])
                    self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                    self.errors += 1
            except KeyError:
                msg = "{} attribute is missing in global attributes".format(attr)
                self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                self.errors += 1
        # -------------------------------------------------------------------
        # Get time axis properties
        # -------------------------------------------------------------------
        # Get calendar and time units
        try:
            calendar = infile.variables['time'].calendar
            timeunits = infile.variables['time'].units
        except BaseException:
            calendar = "gregorian"
            timeunits = "days since ?"
        # Get first and last time bounds

        climatology = self.is_climatology(filename)
        if climatology:
            if cmip6_table.find('Amon') != -1:
                variable = '{}Clim'.format(variable)

        clim_idx = variable.find('Clim')
        if climatology and clim_idx != -1:
            var = [variable[:clim_idx]]

        try:
            if 'bounds' in list(infile.variables['time'].__dict__.keys()):
                bndsvar = infile.variables['time'].__dict__['bounds']
            elif 'climatology' in list(infile.variables['time'].__dict__.keys()):
                bndsvar = infile.variables['time'].__dict__['climatology']
            else:
                bndsvar = 'time_bnds'
            startimebnds = infile.variables[bndsvar][0][0]
            endtimebnds = infile.variables[bndsvar][-1][1]
        except BaseException:
            startimebnds = 0
            endtimebnds = 0

        try:
            startime = infile.variables['time'][0]
            endtime = infile.variables['time'][-1]
        except BaseException:
            startime = 0
            endtime = 0

        # -------------------------------------------------------------------
        # Setup variable
        # -------------------------------------------------------------------
        varid = cmip6_cv.setup_variable(variable_cmor_entry,
                                        cmor_table['variable_entry'][variable_cmor_entry]['units'],
                                        float(cmor_table['Header']['missing_value']),
                                        int(cmor_table['Header']['int_missing_value']),
                                        startime,
                                        endtime,
                                        startimebnds,
                                        endtimebnds)
        if varid == -1:
            msg = "Could not find variable {} in table {} ".format(variable_cmor_entry, cmip6_table)
            self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
            self.prepare_print("└──> :: CV FAIL    :: {}".format(ncfile), 'FAIL', no_text_color)
            raise KeyboardInterrupt
        # -------------------------------------------------------------------
        # Check filename
        # -------------------------------------------------------------------
        if cmip6_cv.check_filename(table, varid, calendar, timeunits, filename) != 0:
            self.errors += 1
        # -------------------------------------------------------------------
        # Check variable attributes
        # -------------------------------------------------------------------
        cv_attrs = cmip6_cv.list_variable_attributes(varid)
        for key in cv_attrs:
            if key == "long_name":
                continue
            if key == "comment":
                continue
            if key == "cell_measures":
                if " OR " in cv_attrs[key] or "OPT" in cv_attrs[key] or "MODEL" in cv_attrs[key] or "UGRID" in cv_attrs[key]:
                    continue
            # Is this attribute in file?
            if key in list(self.dictVar.keys()):
                # Verify that attribute value is equal to file attribute
                table_value = cv_attrs[key]
                file_value = self.dictVar[key]
                # PrePARE accept units of 1 or 1.0 so adjust the table_value
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
                    if abs(table_value - file_value) <= 0.00001 * abs(table_value):
                        table_value = file_value
                if key == "cell_methods":
                    # Remove text that is inside parentheses i.e. comments
                    file_value = re.sub(r"\(.*\)", "", file_value)
                    table_value = re.sub(r"\(.*\)", "", table_value)
                    # Remove extra whitespace
                    file_value = " ".join(file_value.split())
                    table_value = " ".join(table_value.split())
                if key == "cell_measures":
                    # Check if area and volume values from the table's cell_measures are found in the file's external_variables
                    pattern = re.compile('(?:area|volume): (\w+)')
                    values = re.findall(pattern, table_value)
                    for v in values:
                        if 'external_variables' in list(self.dictGbl.keys()):
                            if not re.search(r"\b{}\b".format(v), self.dictGbl['external_variables']):
                                msg = "Your file contains external_variables = \"" + self.dictGbl['external_variables'] + "\", and"
                                if len(values) == 2:
                                    msg += "\nCMIP6 tables requires \"" + values[0] + "\" and \"" + values[1] + "\" in external_variables."
                                else:
                                    msg += "\nCMIP6 tables requires \"" + values[0] + "\" in external_variables."
                                self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                                self.errors += 1
                        else:
                            if len(values) == 2:
                                msg = "Your file contains \"" + values[0] + "\" and \"" + values[1] + "\" in cell_measures and"
                            else:
                                msg = "Your file contains \"" + values[0] + "\" in cell_measures and"
                            msg += "\nCMIP6 tables require attribute \"external_variables\" in global attributes."
                            self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                            self.errors += 1
                        continue

                if str(table_value) != str(file_value):
                    msg =  "Your file contains \"" + key + "\":\"" + str(file_value) + "\" and"
                    msg += "\nCMIP6 tables requires \"" + key + "\":\"" + str(table_value) + "\"."
                    self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                    self.errors += 1
            else:
                # That attribute is not in the file
                table_value = cv_attrs[key]
                if isinstance(table_value, numpy.ndarray):
                    table_value = table_value[0]
                if isinstance(table_value, float):
                    table_value = "{0:.2g}".format(table_value)
                msg = "CMIP6 variable " + variable + " requires \"" + key + "\":\"" + str(table_value) + "\"."
                self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                self.errors += 1
        # Check if cell_measures is defined in the file but not in the table
        if "cell_measures" in list(self.dictVar.keys()) and "cell_measures" not in cv_attrs:
                msg =  "Your file contains \"cell_measures\":\"" + str(self.dictVar["cell_measures"]) + "\" but"
                msg += "\nCMIP6 tables do not define \"cell_measures\"."
                self.prepare_print(msg, 'FAIL', no_text_color, lines=True)
                self.errors += 1
        # Print final message
        if self.errors != 0:
            self.prepare_print("└──> :: CV FAIL    :: {}".format(ncfile), 'FAIL', no_text_color)
            raise KeyboardInterrupt
        elif print_all:
            self.prepare_print("     :: CV SUCCESS :: {}".format(ncfile), 'OKGREEN', no_text_color)


def process(source):
    # Redirect all print statements to a logfile dedicated to the current
    # process
    logfile = '/tmp/PrePARE-{}.log'.format(os.getpid())
    with RedirectedOutput(logfile):
        rc = sequential_process(source)
    # Close and return logfile
    return logfile, rc


def sequential_process(source):
    # Get context from global process env
    assert 'pctx' in list(globals().keys())
    pctx = globals()['pctx']
    try:
        # Process file
        checker = checkCMIP6(pctx.table_path)
        if pctx.variable:
            checker.ControlVocab(source, variable=pctx.variable, print_all=pctx.all, no_text_color=pctx.no_text_color)
        else:
            checker.ControlVocab(source, print_all=pctx.all, no_text_color=pctx.no_text_color)
        return 0
    except KeyboardInterrupt:
        return 1
    except Exception as e:
        print(e)
        msg = "└──> :: SKIPPED    :: {}".format(source)
        if pctx.no_text_color:
            print(msg)
        else:
            print('{}{}{}'.format(BCOLORS.WARNING, msg, BCOLORS.ENDC))
        return 1
    finally:
        # Close opened file
        if hasattr(checker, "infile"):
            checker.infile.close()


def initializer(keys, values):
    """
    Initialize process context by setting particular variables as global variables.

    :param list keys: Argument name list
    :param list values: Argument value list

    """
    assert len(keys) == len(values)
    global pctx
    pctx = ProcessContext({key: values[i] for i, key in enumerate(keys)})


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


def processes_validator(value):
    """
    Validates the max processes number.

    :param str value: The max processes number submitted
    :return:
    """
    pnum = int(value)
    if pnum < 1 and pnum != -1:
        msg = 'Invalid processes number. Should be a positive integer or "-1".'
        raise ArgumentTypeError(msg)
    if pnum == -1:
        # Max processes = None corresponds to cpu.count() in Pool creation
        return None
    else:
        return pnum


#  =========================
#   main()
#  =========================
def main():
    parser = argparse.ArgumentParser(
        prog='PrePARE',
        description='Validate CMIP6 file for ESGF publication.')

    parser.add_argument(
        '-l', '--log',
        metavar='CWD',
        type=str,
        const='{}/logs'.format(os.getcwd()),
        nargs='?',
        help='Logfile directory. Default is the working directory.\n'
             'If not, standard output is used. Only available in multiprocessing mode.')

    parser.add_argument(
        '--variable',
        help='Specify geophysical variable name.\n'
             'If not variable is deduced from filename.')

    parser.add_argument(
        '-v', '--version',
        action='version',
        version='%(prog)s {}.{}.{}'.format(cmip6_cv.CMOR_VERSION_MAJOR,
                                           cmip6_cv.CMOR_VERSION_MINOR,
                                           cmip6_cv.CMOR_VERSION_PATCH),
        help='Version of software.')

    parser.add_argument(
        '--table-path',
        action=DIRECTORYAction,
        default=os.environ['CMIP6_CMOR_TABLES'] if 'CMIP6_CMOR_TABLES' in list(os.environ.keys()) else './Tables',
        help='Specify the CMIP6 CMOR tables path (JSON file).\n'
             'If not submitted read the CMIP6_CMOR_TABLES environment variable if exists.\n'
             'If a directory is submitted table is deduced from filename (default is "./Tables").')

    parser.add_argument(
        '--max-processes',
        metavar='4',
        type=processes_validator,
        default=4,
        help='Number of maximal processes to simultaneously treat several files.\n'
             'Set to one seems sequential processing (default). Set to "-1" seems\n'
             'all available resources as returned by "multiprocessing.cpu_count()".')

    parser.add_argument(
        '--all',
        action='store_true',
        default=False,
        help='Show all results. Default only shows error(s) (i.e., file(s) not compliant)')

    parser.add_argument(
        '--hide-progress',
        action='store_true',
        default=False,
        help='Do not show the percentage of progress / number of files checked while running PrePARE.')

    parser.add_argument(
        '--no-text-color',
        action='store_true',
        default=False,
        help='Remove text color from output.')

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
        nargs='+',
        action=INPUTAction,
        help='Input CMIP6 netCDF data to validate (ex: clisccp_cfMon_DcppC22_NICAM_gn_200001-200001.nc).\n'
             'If a directory is submitted all netCDF recursively found will be validate independently.')

    # Check command-line error
    try:
        args = parser.parse_args()
    except argparse.ArgumentTypeError as errmsg:
        print(str(errmsg), file=sys.stderr)
        return 1
    except SystemExit:
        return 1
    # Get log
    logname = 'PrePARE-{}.log'.format(datetime.now().strftime("%Y%m%d-%H%M%S"))
    log = None
    if args.log:
        if not os.path.isdir(args.log):
            os.makedirs(args.log)
        log = os.path.join(args.log, logname)
    # Collects netCDF files for process
    sources = Collector(args.input)
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
    nb_sources = len(sources)
    errors = 0
    # Init process context
    cctx = dict()
    cctx['no_text_color'] = args.no_text_color
    cctx['table_path'] = args.table_path
    cctx['variable'] = args.variable
    cctx['all'] = args.all
    # Separate sequential process and multiprocessing
    if args.max_processes != 1:
        # Create pool of processes
        pool = Pool(processes=args.max_processes, initializer=initializer, initargs=(list(cctx.keys()), list(cctx.values())))
        # Run processes
        logfiles = list()
        progress = 0
        for logfile, rc in pool.imap(process, sources):
            progress += 1
            percentage = int(progress * 100 / nb_sources)
            if not args.hide_progress:
                if args.no_text_color:
                    msg = '\rCheck netCDF file(s): '
                else:
                    msg = BCOLORS.OKGREEN + '\rCheck netCDF file(s): ' + BCOLORS.ENDC
                msg += '{}% | {}/{} files'.format(percentage, progress, nb_sources)
                sys.stdout.write(msg)
                sys.stdout.flush()
            logfiles.append(logfile)
            errors += rc
        if not args.hide_progress:
            if not args.no_text_color:
                sys.stdout.write('\r\033[K')
            sys.stdout.flush()
        # Print results from logfiles and remove them
        # If --no-text-color is used, then remove the ANSI escape codes from the log output
        remove_ansi = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
        for logfile in set(logfiles):
            if not os.stat(logfile).st_size == 0:
                with open(logfile, 'r') as f:
                    log_text = f.read()
                    if args.no_text_color:
                        log_text = remove_ansi.sub('', log_text)
                    if log:
                        with open(log, 'a+') as r:
                            r.write(log_text)
                    else:
                        sys.stdout.write(log_text)
                        sys.stdout.flush()
            os.remove(logfile)
        # Close pool of processes
        pool.close()
        pool.join()
    else:
        print('Checking data, please wait...')
        initializer(list(cctx.keys()), list(cctx.values()))
        # Print results from logfiles and remove them
        # If --no-text-color is used, then remove the ANSI escape codes from the log output
        remove_ansi = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
        for source in sources:
            logfile, rc = process(source)
            errors += rc
            if not os.stat(logfile).st_size == 0:
                with open(logfile, 'r') as f:
                    log_text = f.read()
                    if args.no_text_color:
                        log_text = remove_ansi.sub('', log_text)
                    if log:
                        with open(log, 'a+') as r:
                            r.write(log_text)
                    else:
                        sys.stdout.write(log_text)
                        sys.stdout.flush()
            os.remove(logfile)
    # Print results summary
    if args.no_text_color:
        msg = '\nNumber of files scanned: {}'.format(nb_sources)
        msg += '\nNumber of file with error(s): {}'.format(errors)
    else:
        msg = BCOLORS.HEADER + '\nNumber of files scanned: {}'.format(nb_sources) + BCOLORS.ENDC
        if errors:
            msg += BCOLORS.FAIL
        else:
            msg += BCOLORS.OKGREEN
        msg += '\nNumber of file with error(s): {}'.format(errors) + BCOLORS.ENDC
    if log:
        with open(log, 'a+') as r:
            r.write(msg)
    print(msg)
    # Evaluate errors and exit with appropriate return code
    if errors != 0:
        if errors == nb_sources:
            # All files has error(s). Error code = -1
            sys.exit(-1)
        else:
            # Some files (at least one) has error(s). Error code = nb files with error(s)
            sys.exit(errors)
    else:
        # No errors. Error code = 0
        sys.exit(0)


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
    main()
