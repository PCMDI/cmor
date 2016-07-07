import csv
import re
import pdb
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
# ==============================================================
#                     deleteLIne()
# ==============================================================
def deleteLine(var_entry, field):
    """
       delete line with <field>  in it
    """
    var_entry = re.sub(r"\n.*" + field + ".*\n", "\n", var_entry)
    return var_entry

# ==============================================================
#                     deleteComa()
# ==============================================================
def deleteComa(var_entry):
    """
       delete last ',' char before '}'
    """
    var_entry = re.sub(r',(\s*)}', r'\n\1}',var_entry)
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
with open('../Tables/CMIP6_expt_list_062116.csv', 'rU') as csvfile:
    spamreader = csv.reader(csvfile, dialect=csv.excel)
    for row in spamreader:
        if row[1] == 'original label':
           break
    expt = ""
    for  row in spamreader:    
##        if (row[5] == "") & (row[22] != ""):
##            print i
        if (row[9] != "" ):
            expt = expt + expt_template
            expt = replaceString(expt, row[9], "experiment_id")
            expt = replaceString(expt, row[17].replace('"', '\''), "title")
            expt = replaceString(expt, row[18].replace('"', '\''), "sub_experiment_id")
            expt = replaceString(expt, row[2], "mip")
            expt = replaceString(expt, row[14], "required_source_type")
            expt = replaceString(expt, row[15], "add_source_type")
            expt = replaceString(expt, row[20], "parent_sub_experiment_id")
            expt = replaceString(expt, row[21], "parent_activity_id")

            if (row[19] != ""):
                expt = replaceString(expt, row[19], "parent_experiment_id")
            else:
                expt = deleteLine(expt, "parent_experiment_id")
                expt = deleteLine(expt, "parent_sub_experiment_id")
                expt = deleteLine(expt, "parent_activity_id")
                expt = deleteLine(expt, "parent_mip_era")
                expt = deleteComa(expt )


#nexpt = expt + "\"Dummy\":{}\n     }"
expt = expt + "\n    }\n}"
expt = deleteComa(expt )
print expt
