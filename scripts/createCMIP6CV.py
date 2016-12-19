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
        "CMIP6_nominal_resolution.json",
        "CMIP6_realm.json",
        "CMIP6_table_id.json",
        "CMIP6_license.json",
        "mip_era.json",
        "CMIP6_experiment_id.json"
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
            print url
            myjson = json.loads(response.read())
            Dico = OrderedDict(Dico.items() + myjson.items())
         
        finalDico = OrderedDict()
        finalDico['CV'] = Dico
        return finalDico

def run():
    f = open("CMIP6_CV.json", "w")
    gather = readWCRP()
    CV = gather.readGit()
    regexp = {}
    regexp["variant_label"] = [ "^r[[:digit:]]\\{1,\\}i[[:digit:]]\\{1,\\}p[[:digit:]]\\{1,\\}f[[:digit:]]\\{1,\\}$" ] 
    regexp["sub_experiment_id"] = [ "^s[[:digit:]]\\{4,4\\}$", "none" ]
    regexp["product"] = [ "output" ] 
    regexp["mip_era"] = [ "CMIP6" ]
    regexp["frequency"] = [ "3hr", "6hr", "day", "fx", "mon", "monClim", "subhr", "yr" ]
    regexp["further_info_url"] = [ "http://furtherinfo.es-doc.org/[[:alpha:]]\\{1,\\}" ]
    regexp["license"] = [
                         "CMIP6 model data produced by .* is licensed under a Creative Commons Attribution \"Share Alike\" 4.0 International License (http://creativecommons.org/licenses/by/4.0/). Use of the data should be acknowledged following guidelines found at.*Permissions beyond the scope of this license may be available at.* Further information about this data, including some limitations, can be found via the further_info_url (recorded as a global attribute in data files). The data producers and data providers make no warranty, either express or implied, including, but not limited to, warranties of merchantability and fitness for a particular purpose. All liabilities arising from the supply of the information (including any liability arising in negligence) are excluded to the fullest extent permitted by law."
                         ],
    CV['CV'] = OrderedDict(CV['CV'].items() + regexp.items())
    f.write(json.dumps(CV, indent=4, separators=(',', ':'), sort_keys=False) )


    f.close()

if __name__ == '__main__':
    run()
