#!/user/bin/env python
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
'''

import cmip6_cv
import cdms2
import argparse
import sys
import os
import json
import numpy


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
# JSONAction()
# =========================


class JSONAction(argparse.Action):
    '''
    Check if argparse is JSON file
    '''

    def __call__(self, parser, namespace, values, option_string=None):
        fn = values
        if not os.path.isfile(fn):
            raise argparse.ArgumentTypeError('JSONAction:{0} is file not found'.format(fn))
        f = open(fn)
        lines = f.readlines()
        jsonobject = json.loads(" ".join(lines))
        if not jsonobject:
            raise argparse.ArgumentTypeError('JSONAction:{0} is file not a valid JSON file'.format(fn))
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
            raise argparse.ArgumentTypeError('CDMSAction:{0} does not exist'.format(fn))
        f = cdms2.open(fn)
        setattr(namespace, self.dest, f)


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
            raise argparse.ArgumentTypeError('readable_dir:{0} is not a valid path'.format(prospective_dir))
        if os.access(prospective_dir, os.R_OK):
            setattr(namespace, self.dest, prospective_dir)
        else:
            raise argparse.ArgumentTypeError('readable_dir:{0} is not a readable dir'.format(prospective_dir))


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
    def __init__(self, args):
        # -------------------------------------------------------------------
        #  Initilaze arrays
        # -------------------------------------------------------------------
        self.cmip6_table = args.cmip6_table
        self.infile     = args.infile
        self.attributes = self.infile.listglobal()
        self.variables  = self.infile.listvariable()
        if args.variable is not None:
            self.var  = [args.variable]
        else:
            # -------------------------------------------------------------------
            # find variable that contains a "history" (should only be one)
            # -------------------------------------------------------------------
            self.var = [var for var in self.variables if 'history' in self.infile.listattribute(var)]

        if((self.var == []) or (len(self.var) > 1)):
            print bcolors.FAIL
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print "! Error:  The input file does not have an history attribute and the CMIP6 variable could not be found"
            print "!         Please use the --variable option to specify your CMIP6 variable"
            print "! Check your file or use CMOR 3.x to achieve compliance for ESGF publication."
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print bcolors.ENDC
            
            raise KeyboardInterrupt

        try:
            self.keys = self.infile.listattribute(self.var[0])
        except:
            print bcolors.FAIL
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print "! Error:  The variable " + self.var[0] + " could not be found"
            print "! Check your file variables "
            print "!!!!!!!!!!!!!!!!!!!!!!!!!"
            print bcolors.ENDC
            
            raise 


        # -------------------------------------------------------------------
        # call setup() to clean all 'C' internal memory.
        # -------------------------------------------------------------------
        cmip6_cv.setup(inpath="../Tables", exit_control=cmip6_cv.CMOR_NORMAL)

        # -------------------------------------------------------------------
        # Set Control Vocabulary file to use (default from cmor.h)
        # -------------------------------------------------------------------
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.GLOBAL_CV_FILENAME, cmip6_cv.TABLE_CONTROL_FILENAME)
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.FILE_PATH_TEMPLATE, cmip6_cv.CMOR_DEFAULT_PATH_TEMPLATE)
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.FILE_NAME_TEMPLATE, cmip6_cv.CMOR_DEFAULT_FILE_TEMPLATE)
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.GLOBAL_ATT_FURTHERINFOURLTMPL, cmip6_cv.CMOR_DEFAULT_FURTHERURL_TEMPLATE)
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.CMOR_AXIS_ENTRY_FILE, "CMIP6_coordinate.json") 
        cmip6_cv.set_cur_dataset_attribute(cmip6_cv.CMOR_FORMULA_VAR_FILE, "CMIP6_formula_terms.json")


        # -------------------------------------------------------------------
        # Create alist of all Global Attributes and set "dataset"
        # -------------------------------------------------------------------
        self.dictGbl = {key: self.infile.getglobal(key) for key in self.attributes}
        ierr = [cmip6_cv.set_cur_dataset_attribute(key, value) for key, value in self.dictGbl.iteritems()]

        # -------------------------------------------------------------------
        # Create a dictionnary of attributes for var
        # -------------------------------------------------------------------
        self.dictVars = dict((y, x) for y, x in
                                    [(key, value) for key in self.keys
                                        if self.infile.getattribute(self.var[0], key) is not None
                                        for value in [self.infile.getattribute(self.var[0], key)]])
        # -------------------------------------------------------------------
        # Load CMIP6 table into memory
        # -------------------------------------------------------------------
        self.table_id = cmip6_cv.load_table(self.cmip6_table)

    def ControlVocab(self):
        '''
            Check CMIP6 global attributes against Control Vocabulary file.

                1. Validate required attribute if presents and some values.
                2. Validate registered institution and institution_id
                3. Validate registered source and source_id
                4. Validate experiment, experiment_id and attribute associated with the experiment.
                5. Validate grid_label and grid_resolution
                6. Validate creation time in ISO format (YYYY-MM-DDTHH:MM:SS)
                7. Validate furtherinfourl from CV internal template
                8. Validate variable attributes with CMOR JSON table.
        '''
        cmip6_cv.check_requiredattributes(self.table_id)
        cmip6_cv.check_institution(self.table_id)
        cmip6_cv.check_sourceID(self.table_id)
        cmip6_cv.check_experiment(self.table_id)
        cmip6_cv.check_grids(self.table_id)
        cmip6_cv.check_ISOTime()
        cmip6_cv.check_furtherinfourl(self.table_id)
        varid = cmip6_cv.setup_variable(self.var[0], 'm', 1e20)

        prepLIST =  cmip6_cv.list_variable_attributes(varid)
        for key in prepLIST:
            if(key == "comment"):
                continue
            # Is this attritue in file?
            if(key in self.dictVars.keys()):
                # Verify that attribute value is equal to file attribute
                table_value = prepLIST[key]
                file_value = self.dictVars[key]
                if isinstance(table_value, numpy.ndarray):
                    table_value = table_value[0]
                if isinstance(file_value, numpy.ndarray):
                    file_value = file_value[0]

                if isinstance(table_value, float):
                    if(table_value / file_value < 1.1):
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
                    print "You file contains \"" + key + "\":\"" + str(file_value) + "\" and"
                    print "CMIP6 tables requires \"" + key + "\":\"" + str(table_value) + "\"."
                    print "====================================================================================="
                    print bcolors.ENDC
                    cmip6_cv.set_CV_Error()
            else:
                # That attribute is not in the file
                table_value = prepLIST[key]
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
        print bcolors.OKGREEN
        print "*************************************************************************************"
        print "* This file is compliant with the CMIP6 specification and can be published in ESGF. *"
        print "*************************************************************************************"
        print bcolors.ENDC


#  =========================
#   main()
#  =========================
def main():
    parser = argparse.ArgumentParser(prog='PrePARE',
                                     description='Validate CMIP6 file '
                                     'for ESGF publication.')

    parser.add_argument('--variable',
                        help='specify geophysical variable name')

    parser.add_argument('cmip6_table',
                        help='CMIP6 CMOR table (JSON file) ex: Tables/CMIP6_Amon.json', 
                        action=JSONAction)

    parser.add_argument('infile',
                        help='Input CMIP6 netCDF file to Validate ex: clisccp_cfMon_DcppC22_NICAM_gn_200001-200001.nc',
                        action=CDMSAction)

    parser.add_argument('outfile',
                        nargs='?',
                        help='Output file (default stdout)',
                        type=argparse.FileType('w'),
                        default=sys.stdout)

    try:
        args = parser.parse_args()
    except argparse.ArgumentTypeError as errmsg:
        print >> sys.stderr, str(errmsg)
        return 1
    except SystemExit:
        return 1

    process = checkCMIP6(args)
    process.ControlVocab()

# process.checkActivities()
    return(0)


if(__name__ == '__main__'):
    try:
        sys.exit(main())

    except KeyboardInterrupt:
        print bcolors.FAIL
        print "!!!!!!!!!!!!!!!!!!!!!!!!!"
        print "! Error:  The input file is not CMIP6 compliant"
        print "! Check your file or use CMOR 3.x to achieve compliance for ESGF publication."
        print "!!!!!!!!!!!!!!!!!!!!!!!!!"
        print bcolors.ENDC
        sys.exit(-1)
    except:
        sys.exit(-1)
