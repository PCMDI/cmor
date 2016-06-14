# -*- coding: utf-8 -*-
'''
Created on Fri Feb 19 11:33:52 2016

@author: Denis Nadeau LLNL
'''

import cdms2
import argparse
import sys
import os
import json
import pdb

EXPERIMENTS = 'Tables/experiments.json'
CONTROLVOCABULARY = 'Tables/CV.json'


# =========================
# JSONAction()
# =========================
class JSONAction(argparse.Action):
    '''
    Check if argparse is JSON file
    '''

    def __call__(self, parser, namespace, values, option_string=None):
        pdb.set_trace()
        fn = values
        if not os.path.isfile(fn):
            raise argparse.ArgumentTypeError('JSONAction:{0} is file not found'.format(fn))
        f = open(fn)
        lines=f.readlines()
        jsonobject=json.loads(" ".join(lines))
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
        #pdb.set_trace()
        self.expfile = EXPERIMENTS
        self.cmip6_table =  args.cmip6_table
        self.CVfn =  args.CV

        self.infile = args.infile
        # -------------------------
        # Load experiment.json
        # -------------------------
        if(os.path.isfile(self.expfile)):
            f = open(self.expfile).read()
            self.experiment = json.loads(f)
        # -------------------------
        # Load CMIP6 JSON table
        # -------------------------
        if(os.path.isfile(self.cmip6_table)):
            f = open(self.cmip6_table).read()
            self.table = json.loads(f)
        # -------------------------
        # Load CMIP6 Control Vocabulary JSON
        # -------------------------
        if(os.path.isfile(self.CVfn) is False):
            self.CVfn = args.CV
            
        if(os.path.isfile(self.CVfn)):
            f = open(self.CVfn).read()
            self.CV = json.loads(f)

    # *************************
    #   ControlVocab()
    # *************************
    def ControlVocab(self, Element, Control):
        '''
        '''
        # -------------------------------------------------
        # We have a dictionary pass on the keys recursively
        # -------------------------------------------------
        if(isinstance(Control, dict)):
            bInList = self.ControlVocab(Element, Control.keys())
            return(bInList)
        # -------------------------------------------------
        # Check if Elment is in a list
        # -------------------------------------------------
        elif(isinstance(Element, str) and isinstance(Control, list)):
            bInList = Element in Control
            return(bInList)
        # -------------------------------------------------
        # Check if Elment is in a list
        # -------------------------------------------------
        elif(isinstance(Element, list) and isinstance(Control, list)):
            ControlVocab = [ControlItem  for ControlItem in Control
                            if ControlItem not in Element]
            return(ControlVocab)

        # -------------------------------------------------
        # File instance check global attributes with list
        # -------------------------------------------------
        elif(isinstance(Element, cdms2.dataset.CdmsFile) and isinstance(Control, list)):
            ControlVocab = [ControlItem  for ControlItem in Control
                            if ControlItem not in Element.__dict__.keys()]
            return(ControlVocab)
        # -------------------------------------------------
        # Variableinstance check dimensions attribute and min/max
        # -------------------------------------------------
        elif(isinstance(Element, cdms2.fvariable.FileVariable) and isinstance(Control, list)):
            pass

    # *************************
    #   CheckExperiments()
    # *************************
    def checkExperiments(self):
        '''
        Control experiment_id and experiment
        '''
        #pdb.set_trace()
        bValid = self.ControlVocab(self.infile.experiment_id, self.experiment['experiments'])
        if(not bValid):
            print "{0} not found in {1}".format(self.infile.experiment_id, self.expfile)
        bValid = self.ControlVocab(self.infile.experiment, [self.experiment['experiments'][self.infile.experiment_id]])
        if(not bValid):
            print "{0} not valid experiment definitin check {1}".format(self.infile.experiment, self.expfile)

    # *************************
    #   CheckGlobalAttributes()
    # *************************
    def checkRequiredGlobalAttributes(self):
        '''
        Control Global Attributes.
        '''
        pdb.set_trace()
        Control = self.ControlVocab(self.infile.attributes.keys(),
                                    self.CV['CV']['required_global_attributes'])

        if(Control):
            for Vocab in Control:
                print "Global Attribute '{0}' not in CMIP6 file".format(Vocab)
            if(Vocab.find("source") != -1):
                print "coucou"
                pdb.set_trace()
                print "coucou"



#  =========================
#   main()
#  =========================
def main():
    parser = argparse.ArgumentParser(prog='CMIP6Checker',
                                     description='Validate CMIP6 file '
                                     'for publication in ESGF.')

    parser.add_argument('cmip6_table',
                        help='CMIP6 CMOR table (JSON file) ex: Tables/CMIP6_Amon.json', action=JSONAction)

    parser.add_argument('CV',
                        help='Control Vocabulary (CV.json) ex: Tables/CV.json',
                        default='CV.json',action=JSONAction)

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
    except argparse.ArgumentTypeError, errmsg:
        print >> sys.stderr, str(errmsg)
        return 1
    except SystemExit:
        return 1

    pdb.set_trace()
    process = checkCMIP6(args)
    process.checkExperiments()
    process.checkRequiredGlobalAttributes()
# process.checkActivities()
    return(0)


if(__name__ == '__main__'):
    sys.exit(main())
