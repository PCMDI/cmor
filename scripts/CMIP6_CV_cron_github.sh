#!/bin/bash -x
export REPOS_PATH=${1:-"${HOME}/CVs_tmp"}
echo "Assuming REPOs cloned in "${REPOS_PATH}
cd ${REPOS_PATH}/cmor/scripts
git pull
python createCMIP6CV.py
cd ${REPOS_PATH}/cmip6-cmor-tables/Tables
git pull
cp -v ${REPOS_PATH}/cmor/scripts/CMIP6_CV.json  .
msg="cron: update CMIP6_CV -- "`date +%Y-%m-%dT%H:%M`
echo $msg
git commit -am '$msg'
git push