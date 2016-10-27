import os
import sys
import json
import pdb
import urllib
from collections import OrderedDict


# List of files needed from github for CMIP6 CV
# ---------------------------------------------
filelist = [ 
        "CMIP6_required_global_attributes.json",
        "CMIP6_activity_id.json",
        "CMIP6_institution_id.json",
        "CMIP6_source_id.json",
        "CMIP6_source_type.json",
        "CMIP6_frequency.json",
        "CMIP6_grid_label.json",
        "CMIP6_grid_resolution.json",
        "CMIP6_realm.json",
        "CMIP6_table_id.json",
        "CMIP6_experiment_id.json",
        "CMIP6_license.json",
        "mip_era.json"
        ]
# Github repository with CMIP6 related Control Vocabulary files
# -------------------------------------------------------------
githubRepo = "https://raw.githubusercontent.com/WCRP-CMIP/CMIP6_CVs/master/"

class readWCRP():
    def __init__(self):
        pass

    def readGit(self):
        Dico = OrderedDict()
        for file in filelist:
            url = githubRepo + file 
            response = urllib.urlopen(url)
            myjson = json.loads(response.read())
            Dico = OrderedDict(Dico.items() + myjson.items())
         
        finalDico = OrderedDict()
        finalDico['CV'] = Dico
        return finalDico

def run():
    f = open("CMIP6_CV.json", "w")
    gather = readWCRP()
    CV = gather.readGit()
    f.write(json.dumps(CV, indent=4, separators=(',', ':'), sort_keys=False) )
    f.close()

if __name__ == '__main__':
    run()
