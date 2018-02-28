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

'''
Created on Fri Feb 19 11:33:52 2016

@author: Denis Nadeau LLNL
@co-author: Guillaume Levavasseur (IPSL) Parallelization
'''
import re
import sys
from contextlib import contextmanager

# Make sure cdms2.__init__py is not loaded when importing Cdunif
sys.path.insert(0, sys.prefix + "/lib/python2.7/site-packages/cdms2")
import Cdunif
import argparse
import os
import json
import numpy
import cmip6_cv
from multiprocessing import Pool


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[1;32m'
    WARNING = '\033[1;34;47m'
    FAIL = '\033[1;31;47m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

# =========================
# FILEAction()
# =========================
class FILEAction(argparse.Action):
    '''
    Check if argparse is JSON file
    '''

    def __call__(self, parser, namespace, values, option_string=None):
        fn = values
        if not os.path.isfile(fn):
            raise argparse.ArgumentTypeError(
                'FILEAction:{0} is file not found'.format(fn))
        f = open(fn)
        lines = f.readlines()
        f.close()
        setattr(namespace, self.dest, lines)

# =========================
# JSONAction()
# =========================
class JSONAction(argparse.Action):
    '''
    Check if argparse is JSON file
    '''

    def __call__(self, parser, namespace, values, option_string=None):
        fn = values
        if not os.path.isfile(fn):
            raise argparse.ArgumentTypeError(
                'JSONAction:{0} is file not found'.format(fn))
        f = open(fn)
        lines = f.readlines()
        f.close()
        jsonobject = json.loads(" ".join(lines))
        if not jsonobject:
            raise argparse.ArgumentTypeError(
                'JSONAction:{0} is file not a valid JSON file'.format(fn))
        setattr(namespace, self.dest, values)


# =========================
# CDMSAction()
# =========================
class CDMSAction(argparse.Action):
    '''
    Check if argparse is CDMS file
    '''

    def __call__(self, parser, namespace, values, option_string=None):
        fn = values
        if not os.path.isfile(fn):
            raise argparse.ArgumentTypeError(
                'CDMSAction:{0} does not exist'.format(fn))
        f = Cdunif.CdunifFile(fn, "r")
        f.close()
        setattr(namespace, self.dest, fn)


# =========================
# readable_dir()
# =========================
class readable_dir(argparse.Action):
    '''
    Check if argparse is a directory.
    '''

    def __call__(self, parser, namespace, values, option_string=None):
        prospective_dir = values
        if not os.path.isdir(prospective_dir):
            raise argparse.ArgumentTypeError(
                'readable_dir:{0} is not a valid path'.format(prospective_dir))
        if os.access(prospective_dir, os.R_OK):
            setattr(namespace, self.dest, prospective_dir)
        else:
            raise argparse.ArgumentTypeError(
                'readable_dir:{0} is not a readable dir'.format(prospective_dir))


# =========================
# DIRAction()
# =========================
class DIRECTORYAction(argparse.Action):
    '''
    Check if argparse is a directory.
    '''

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
    '''
    Checks if the supplied input exists.
    '''

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
        assert isinstance(self.sources, list)

    def __iter__(self):
        for source in self.sources:
            if os.path.isdir(source):
                # If input is a directory: walk through it and yields netCDF
                # files
                for root, _, filenames in os.walk(source, followlinks=True):
                    for filename in sorted(filenames):
                        ffp = os.path.join(root, filename)
                        if os.path.isfile(ffp) and re.search(
                                re.compile('^.*\.nc$'), filename):
                            yield (ffp, self.data)
            else:
                # It input is a file: yields the netCDF file itself
                yield (source, self.data)


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
    '''
    Validate if a file is CMIP6 compliant and ready for publication.

    Class need to read CMIP6 Table and Controled Vocabulary file.

    As well,the class will load the EXPERIMENT json file

    Input:
        args.cmip6_table:  CMIP6 table used to creat this file,
                           variable attributes and dimensions will be controled.
        args.CV:           Controled Vocabulary "json" file.

    Output:
        outfile:      Log file, default is stdout.

    '''

    # *************************
    #   __init__()
    # *************************
    def __init__(self, table_path):
        # -------------------------------------------------------------------
        #  Initilaze table path
        # -------------------------------------------------------------------
        self.cmip6_table_path = os.path.normpath(table_path)
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
    def _check_JSON_table(path):
        f = open(path)
        lines = f.readlines()
        f.close()
        jsonobject = json.loads(" ".join(lines))
        if not jsonobject:
            raise argparse.ArgumentTypeError(
                'Invalid JSON CMOR table: {}'.format(path))

    def setDoubleValue(self, attribute):
        if (cmip6_cv.has_cur_dataset_attribute(attribute)):
            if (isinstance(self.dictGbl[attribute], numpy.ndarray) and isinstance(self.dictGbl[attribute][0],
                                                                                  numpy.float64)):
                self.dictGbl[attribute] = self.dictGbl[attribute][0]
                cmip6_cv.set_cur_dataset_attribute(
                    attribute, self.dictGbl[attribute])

    def ControlVocab(self, ncfile):
        '''
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
               10. Validate sub_experiment_* atributes.
               11. Validate that all *_index are integers.
        '''
        filename = os.path.basename(ncfile)
        # -------------------------------------------------------------------
        #  Initilaze arrays
        # -------------------------------------------------------------------
        if os.path.isfile(self.cmip6_table_path):
            self.cmip6_table = self.cmip6_table_path
        else:
            self.cmip6_table = '{}/CMIP6_{}.json'.format(
                self.cmip6_table_path, self._get_table_from_filename(filename))

        self._check_JSON_table(self.cmip6_table)
        # -------------------------------------------------------------------
        # Load CMIP6 table into memory
        # -------------------------------------------------------------------
        self.table_id = cmip6_cv.load_table(self.cmip6_table)
        # -------------------------------------------------------------------
        #  Deduce variable
        # -------------------------------------------------------------------
        self.variable = self._get_variable_from_filename(filename)
        climatology = False
        if( filename.find('-clim') != -1 ):
            climatology = True
            if( self.cmip6_table.find('Amon') != -1):
                self.variable = self.variable + 'Clim'

        # -------------------------------------------------------------------
        #  Open file in processing
        # -------------------------------------------------------------------
        self.infile = Cdunif.CdunifFile(ncfile, "r")
        # -------------------------------------
        # Create alist of all Global Attributes
        # -------------------------------------
        self.dictGbl = {key: self.infile.__dict__[
            key] for key in self.infile.__dict__.keys()}
        self.attributes = self.infile.__dict__.keys()
        self.variables = self.infile.variables.keys()
        ierr = [
            cmip6_cv.set_cur_dataset_attribute(
                key,
                value) for key,
            value in self.dictGbl.iteritems()]
        member_id = ""
        if ("sub_experiment_id" in self.dictGbl.keys()):
            if (self.dictGbl["sub_experiment_id"] not in ["none"]):
                member_id = self.dictGbl["sub_experiment_id"] + \
                    '-' + self.dictGbl["variant_label"]
            else:
                member_id = self.dictGbl["variant_label"]
        cmip6_cv.set_cur_dataset_attribute(
            cmip6_cv.GLOBAL_ATT_MEMBER_ID, member_id)
        self.setDoubleValue('branch_time_in_parent')
        self.setDoubleValue('branch_time_in_child')
        if self.variable is not None:
            self.var = [self.variable]
        else:
            # -------------------------------------------------------------------
            # find variable that contains a "history" (should only be one)
            # -------------------------------------------------------------------
            self.var = [self.infile.variable_id]

        climPos = self.var[0].find('Clim')
        if climatology and climPos != -1: 
            self.var = [self.var[0][:climPos]]

        if ((self.var == []) or (len(self.var) > 1)):
            print bcolors.FAIL
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print "! Error:  The input file does not have an history attribute and the CMIP6 variable could not be found"
            print "!         Please use the --variable option to specify your CMIP6 variable"
            print "! Check your file or use CMOR 3.x to achieve compliance for ESGF publication."
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print bcolors.ENDC
            raise KeyboardInterrupt

        try:
            self.keys = self.infile.variables[self.var[0]].__dict__.keys()
        except BaseException:
            print bcolors.FAIL
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print "! Error:  The variable " + self.var[0] + " could not be found"
            print "! Check your file variables "
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print bcolors.ENDC
            raise KeyboardInterrupt

        # -------------------------------------------------------------------
        # Create a dictionnary of attributes for var
        # -------------------------------------------------------------------
        self.dictVars = dict((y, x) for y, x in
                             [(key, value) for key in self.keys
                              if self.infile.variables[self.var[0]].__dict__[key] is not None
                              for value in [self.infile.variables[self.var[0]].__dict__[key]]])
        try:
            self.calendar = self.infile.variables['time'].calendar
            self.timeunits = self.infile.variables['time'].units
        except BaseException:
            self.calendar = "gregorian"
            self.timeunits = "days since ?"
        cmip6_cv.check_requiredattributes(self.table_id)
        cmip6_cv.check_institution(self.table_id)
        cmip6_cv.check_sourceID(self.table_id)
        cmip6_cv.check_experiment(self.table_id)
        cmip6_cv.check_grids(self.table_id)
        cmip6_cv.check_ISOTime()
        cmip6_cv.check_furtherinfourl(self.table_id)
        cmip6_cv.check_parentExpID(self.table_id)
        cmip6_cv.check_subExpID(self.table_id)

        try:
            if climatology:
                startimebnds = self.infile.variables['climatology_bnds'][0][0]
                endtimebnds = self.infile.variables['climatology_bnds'][-1][1]
            else:
                startimebnds = self.infile.variables['time_bnds'][0][0]
                endtimebnds = self.infile.variables['time_bnds'][-1][1]
        except BaseException:
            startimebnds = 0
            endtimebnds = 0
         
        try:
            startime = self.infile.variables['time'][0]
            endtime = self.infile.variables['time'][-1]
        except BaseException:
            startime = 0
            endtime = 0

        varunits = self.infile.variables[self.var[0]].units
        varmissing = self.infile.variables[self.var[0]]._FillValue[0]
        # -------------------------------------------------
        # Make sure with use self.variable for Climatology
        # -------------------------------------------------
        varid = cmip6_cv.setup_variable(self.variable, varunits, varmissing, startime, endtime,
                                        startimebnds, endtimebnds)
        if (varid == -1):
            print bcolors.FAIL
            print "====================================================================================="
            print " Could not find variable '%s' in table '%s' " % (self.var[0], self.cmip6_table)
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()
            raise KeyboardInterrupt

        fn = os.path.basename(str(self.infile).split('\'')[1])
        cmip6_cv.check_filename(
            self.table_id,
            varid,
            self.calendar,
            self.timeunits,
            fn)

        if not isinstance(self.dictGbl['branch_time_in_child'], numpy.float64):
            print bcolors.FAIL
            print "====================================================================================="
            print "branch_time_in_child is not a double: ", type(self.dictGbl['branch_time_in_child'])
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()

        if not isinstance(self.dictGbl['branch_time_in_parent'], numpy.float64):
            print bcolors.FAIL
            print "====================================================================================="
            print "branch_time_in_parent is not an double: ", type(self.dictGbl['branch_time_in_parent'])
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()

        if not isinstance(self.dictGbl['realization_index'], numpy.ndarray):
            print bcolors.FAIL
            print "====================================================================================="
            print "realization_index is not an integer: ", type(self.dictGbl['realization_index'])
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()

        if not isinstance(self.dictGbl['initialization_index'], numpy.ndarray):
            print bcolors.FAIL
            print "====================================================================================="
            print "initialization_index is not an integer: ", type(self.dictGbl['initialization_index'])
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()

        if not isinstance(self.dictGbl['physics_index'], numpy.ndarray):
            print bcolors.FAIL
            print "====================================================================================="
            print "physics_index is not an integer: ", type(self.dictGbl['physics_index'])
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()

        if not isinstance(self.dictGbl['forcing_index'], numpy.ndarray):
            print bcolors.FAIL
            print "====================================================================================="
            print "forcing_index is not an integer: ", type(self.dictGbl['forcing_index'])
            print "====================================================================================="
            print bcolors.ENDC
            cmip6_cv.set_CV_Error()

        # -----------------------------
        # variable attribute comparison
        # -----------------------------
        prepLIST = cmip6_cv.list_variable_attributes(varid)
        for key in prepLIST:
            if(key == "long_name"):
                continue
            if(key == "comment"):
                continue
            # Is this attritue in file?
            if(key in self.dictVars.keys()):
                # Verify that attribute value is equal to file attribute
                table_value = prepLIST[key]
                file_value = self.dictVars[key]

                # PrePARE accept units of 1 or 1.0 so adjust thet table_value
                # -----------------------------------------------------------
                if(key == "units"):
                   if((table_value == "1") and (file_value == "1.0")):
                       table_value = "1.0"
                   if((table_value == "1.0") and (file_value == "1")):
                       table_value = "1"

                if isinstance(table_value, str) and isinstance(file_value, numpy.ndarray):
                   if(numpy.array([int(value) for value in table_value.split()] == file_value).all()):
                       file_value=True
                       table_value=True

                if isinstance(table_value, numpy.ndarray):
                    table_value = table_value[0]
                if isinstance(file_value, numpy.ndarray):
                    file_value = file_value[0]
                if isinstance(table_value, float):
                    if(file_value == 0):
                        if(table_value != file_value):
                            file_value = False
                    else:
                        if(1 - (table_value / file_value) < 0.00001):
                            table_value = file_value

                if key == "cell_methods":
                    idx = file_value.find(" (interval:")
                    file_value = file_value[:idx]
                    table_value = table_value[:idx]

                file_value = str(file_value)
                table_value = str(table_value)
                if table_value != file_value:
                    print bcolors.FAIL
                    print "====================================================================================="
                    print "Your file contains \"" + key + "\":\"" + str(file_value) + "\" and"
                    print "CMIP6 tables requires \"" + key + "\":\"" + str(table_value) + "\"."
                    print "====================================================================================="
                    print bcolors.ENDC
                    cmip6_cv.set_CV_Error()
            else:
                # That attribute is not in the file
                table_value = prepLIST[key]
                if key == "cell_measures":
                    if((table_value.find("OPT") != -1) or (table_value.find("MODEL") != -1)):
                        continue
                if isinstance(table_value, numpy.ndarray):
                    table_value = table_value[0]
                if isinstance(table_value, float):
                    table_value = "{0:.2g}".format(table_value)
                print bcolors.FAIL
                print "====================================================================================="
                print "CMIP6 variable " + self.var[0] + " requires \"" + key + "\":\"" + str(table_value) + "\"."
                print "====================================================================================="
                print bcolors.ENDC
                cmip6_cv.set_CV_Error()

        if(cmip6_cv.get_CV_Error()):
            raise KeyboardInterrupt
        else:
            print bcolors.OKGREEN
            print "*************************************************************************************"
            print "* This file is compliant with the CMIP6 specification and can be published in ESGF  *"
            print "*************************************************************************************"
            print bcolors.ENDC


def process(source):
    # Redirect all print statements to a logfile dedicated to the current
    # process
    logfile = '/tmp/PrePARE-{}.log'.format(os.getpid())
    with RedirectedOutput(logfile):
        try:
            # Deserialize inputs
            ncfile, table_path = source
            print "Processing: {}\n".format(ncfile)
            # Process file
            checker = checkCMIP6(table_path)
            checker.ControlVocab(ncfile)
        except KeyboardInterrupt:
            print bcolors.FAIL
            print "*************************************************************************************"
            print "* Error: The input file is not CMIP6 compliant                                      *"
            print "* Check your file or use CMOR 3.x to achieve compliance for ESGF publication        *"
            print "*************************************************************************************"
            print bcolors.ENDC
        finally:
            # Close opened file
            if hasattr(checker, "infile"):
                checker.infile.close()
    # Close and return logfile
    return logfile


def sequential_process(source):
    try:
        # Deserialize inputs
        ncfile, table_path = source
        print "Processing: {}\n".format(ncfile)
        # Process file
        checker = checkCMIP6(table_path)
        checker.ControlVocab(ncfile)
    except KeyboardInterrupt:
        print bcolors.FAIL
        print "*************************************************************************************"
        print "* Error: The input file is not CMIP6 compliant                                      *"
        print "* Check your file or use CMOR 3.x to achieve compliance for ESGF publication        *"
        print "*************************************************************************************"
        print bcolors.ENDC
    finally:
        # Close opened file
        if hasattr(checker, "infile"):
            checker.infile.close()

#  =========================
#   main()
#  =========================


def main():
    parser = argparse.ArgumentParser(prog='PrePARE',
                                     description='Validate CMIP6 file '
                                                 'for ESGF publication.')

    parser.add_argument('--variable',
                        help='specify geophysical variable name')


    parser.add_argument('--table_path',
                        help='Specify the CMIP6 CMOR tables path (JSON file).'
                             'Default is "./Tables".',
                        action=DIRECTORYAction,
                        default='./Tables')

    parser.add_argument('--max-threads',
                        type=int,
                        default=1,
                        help='Number of maximal threads to simultaneously process several files.'
                             'Default is one as sequential processing.')

    parser.add_argument('cmip6_table',
                        help='Specify the CMIP6 CMOR tables path (JSON file) or CMIP6 table file.'
                             'Default is "./Tables".',
                        action=DIRECTORYAction,
                        default='./Tables')

    parser.add_argument('input',
                        help='Input CMIP6 netCDF data to validate (ex: clisccp_cfMon_DcppC22_NICAM_gn_200001-200001.nc.'
                             'If a directory is submitted all netCDF recusively found will be validate independently.',
                        nargs='+',
                        action=INPUTAction)

#    parser.add_argument('outfile',
#                        help='Output file (default stdout)',
#                        type=argparse.FileType('w'),
#                        default=sys.stdout)

    # Check command-line error
    try:
        args = parser.parse_args()
    except argparse.ArgumentTypeError as errmsg:
        print >> sys.stderr, str(errmsg)
        return 1
    except SystemExit:
        return 1

    if hasattr(args,"table_path"):
        cmip6_table=args.table_path

    # Collects netCDF files for process
    sources = Collector(args.input, data=args.cmip6_table)
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
        for logfile in set(logfiles):
            with open(logfile, 'r') as f:
                print f.read()
            os.remove(logfile)
        # Close pool of processes
        pool.close()
        pool.join()
    else:
        for source in sources:
            sequential_process(source)


@contextmanager
def RedirectedOutput(to=os.devnull):
    fd_out = sys.stdout.fileno()
    old_stdout = os.fdopen(os.dup(fd_out), 'w')
    fd_err = sys.stderr.fileno()
    old_stderr = os.fdopen(os.dup(fd_err), 'w')
    stream = open(to, 'w')
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


if (__name__ == '__main__'):
    sys.exit(main())
