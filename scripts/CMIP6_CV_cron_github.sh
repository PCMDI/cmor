#!/bin/bash
export PATH=/software/anaconda2/envs/cmor3/bin:/software/anaconda2/bin:/usr/local/bin:/usr/bin:/bin
alias git='hub'
cd /software/cmor3/cmor/scripts
python /software/cmor3/cmor/scripts/createCMIP6CV.py
# -----
cd /software/cmip6-cmor-tables/Tables
git checkout CMIP6_CVupdt
cp -v /software/cmor3/cmor/scripts/CMIP6_CV.json  .
msg="cron: update CMIP6_CV -- "`date +%Y-%m-%dT%H:%M`
echo $msg
git commit -am "$msg"
git push
URL=`hub pull-request CMIP6_CVupdt -m "$msg"`
git checkout master
git pull origin
hub merge $URL
git push origin

#
