import csv
import re
# ==============================================================
#                     replaceString()
# ==============================================================
def replaceString(var_entry, var, field):
    """
       Replace a <field> with a string (deleting "<" and ">")
    """
    if(var == ''):
        var_entry = re.sub(r"" + field + ":.*\n", "", var_entry)
    var_entry = var_entry.replace("<" + field + ">", var)
    return var_entry


expt_template ="""
        "<experiment_id>": {
                               "experiment":                "<title>",
                               "sub_experiment_id":         "<sub_experiment_id>",
                               "activity_id":               "<mip>",
                               "mip_era":                   "CMIP6",
                               "source_type":               "<required_source_type>",
                               "additional_source_type":    "<add_source_type>",
                               "parent_experiment_id" :     "<parent_experiment_id>",
                               "parent_sub_experiment_id":  "<parent_sub_experiment_id>",
                               "parent_activity_id ":       "<parent_activity_id>",
                               "parent_mip_era ":           "CMIP6"
                          },
"""
print "{"
print "    \"experiment_ids\": { "
with open('../Tables/CMIP6_expt_list_042716-1.csv', 'rU') as csvfile:
    spamreader = csv.reader(csvfile, dialect=csv.excel)
    for row in spamreader:
        if row[5] == 'original label':
           break
    expt = ""
    i=13
    for  row in spamreader:    
##        if (row[5] == "") & (row[22] != ""):
##            print i
        if (row[14] != "" ):
            expt = expt + expt_template
            expt = replaceString(expt, row[14], "experiment_id")
            expt = replaceString(expt, row[22].replace('"', '\''), "title")
            expt = replaceString(expt, row[18].replace('"', '\''), "sub_experiment_id")
            expt = replaceString(expt, row[7], "mip")
            expt = replaceString(expt, row[19], "required_source_type")
            expt = replaceString(expt, row[20], "add_source_type")
            expt = replaceString(expt, row[23], "parent_experiment_id")
            expt = replaceString(expt, row[24], "parent_sub_experiment_id")
            expt = replaceString(expt, row[25], "parent_activity_id")
            i=i+1

expt = expt + "\"Dummy\":{}\n     }"
print expt
print "}"
