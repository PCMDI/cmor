#!/bin/bash -x

# Store existing UDUNITS2 env vars and set to this conda env
# so other CMOR installs don't pollute the environment

if [[ -n "$UDUNITS2_XML_PATH" ]]; then
    export _CONDA_SET_UDUNITS2_XML_PATH=${UDUNITS2_XML_PATH}
fi
export UDUNITS2_XML_PATH=${CONDA_PREFIX}/share/udunits/udunits2.xml

