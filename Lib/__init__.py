import os
import sys
xml_pth = os.path.join(sys.prefix,"share","udunits","udunits2.xml")
if os.path.exists(xml_pth):
    os.environ["UDUNITS2_XML_PATH"] = xml_pth

from cmor_const import *

from pywrapper import axis,variable,write,setup,load_table,set_table,zfactor,close,grid,set_grid_mapping,time_varying_grid_coordinate,set_cur_dataset_attribute,get_cur_dataset_attribute,has_cur_dataset_attribute,create_output_path,set_variable_attribute,get_variable_attribute,has_variable_attribute,dataset_json,get_final_filename,set_deflate

try:
  from check_CMOR_compliant import checkCMOR
except:
  pass
