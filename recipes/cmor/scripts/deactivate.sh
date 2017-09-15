#!/bin/bash
# Restore previous CMOR env vars if they were set

unset UDUNITS2_XML_PATH
if [[ -n "$_CONDA_SET_UDUNITS2_XML_PATH" ]]; then
    export UDUNITS2_XML_PATH=${_CONDA_SET_UDUNITS2_XML_PATH}
    unset _CONDA_SET_UDUNITS2_XML_PATH
fi

