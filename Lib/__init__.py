import os
import sys
xml_pth = os.path.join(sys.prefix, "share", "udunits", "udunits2.xml")
if os.path.exists(xml_pth):
    os.environ["UDUNITS2_XML_PATH"] = xml_pth

from cmor_const import *

from pywrapper import (
    CMORError, axis, variable, write, setup, load_table, set_table, zfactor,
    close, grid, set_grid_mapping, time_varying_grid_coordinate, dataset_json,
    set_cur_dataset_attribute, get_cur_dataset_attribute,
    has_cur_dataset_attribute, set_variable_attribute, get_variable_attribute,
    has_variable_attribute, get_final_filename, set_deflate, set_furtherinfourl, 
    set_climatology, get_climatology, set_terminate_signal, get_terminate_signal)

try:
    from check_CMOR_compliant import checkCMOR
except ImportError:
    pass
