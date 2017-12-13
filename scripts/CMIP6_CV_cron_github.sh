#!/bin/bash -x
export PATH=~/anaconda2/bin:/usr/local/bin:/usr/bin:/bin
alias git='hub'
cd ~/cmor/scripts
python createCMIP6CV.py
# -----
cd ~/cmip6-cmor-tables/Tables
git checkout master
git pull --all -p
git checkout CMIP6_CVupdt
git pull
git merge master
git push 
cp -v ~/cmor/scripts/CMIP6_CV.json  .
msg="cron: update CMIP6_CV -- "`date +%Y-%m-%dT%H:%M`
echo $msg
git commit -am $msg
git push
URL=`hub pull-request CMIP6_CVupdt -m $msg`
if [[ ! -z $URL ]]; then
	PR=`echo $URL | awk -F/ '{print $7}'`
	TOKEN=`cat ~/.config/hub  | grep oauth_token | awk -F: '{print $2}' | sed -e 's/^\s*//'`
	curl "https://api.github.com/repos/pcmdi/cmip6-cmor-tables/pulls/$PR/merge"   -XPUT   -H "Authorization: token $TOKEN"   -H "Accept: application/vnd.github.polaris-preview"   -d '{ "commit_message":"$msg", "squash": true, "commit_title": "$msg" }'
	git checkout master
	git pull origin
fi
#
