# Updating the CV

## Make sure tables from DRS are up to date

Follow directions at: 
https://github.com/PCMDI/xml-cmor3-database/blob/master/README.md

## Update CV

Essentially:

Fisrt clone the repos cmor and cmip6-cmor-tables
```bash
export REPOS_PATH=${1:-"${HOME}/CVs_tmp"}
echo "Cloning to "${REPOS_PATH}
cd ${REPOS_PATH}
git clone git://github.com/pcmdi/cmip6-cmor-tables
git clone git://github.com/pcmdi/cmor
```

Then run the CMIP6_CV_cron_github.sh script

```bash
bash CMIP6_CV_cron_github.sh
```
