#!/usr/bin/env bash
PKG_NAME=cmor
USER=pcmdi
echo "Trying to upload conda"
if [ `uname` == "Linux" ]; then
    OS=linux-64
    echo "Linux OS"
    yum install -y wget git gcc
    if [ $CMOR_PYTHON_VERSION == '2.7' ]; then 
        wget --no-check https://repo.continuum.io/miniconda/Miniconda2-4.3.30-Linux-x86_64.sh -O miniconda2.sh
        bash miniconda2.sh -b -p ${HOME}/miniconda
    fi
    if [ $CMOR_PYTHON_VERSION == '3.6' || $CMOR_PYTHON_VERSION == '3.7' ]; then 
        wget --no-check https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda3.sh
        bash miniconda3.sh -b -p ${HOME}/miniconda
    fi
    export SYSPATH=$PATH
    export PATH=${HOME}/miniconda/bin:${SYSPATH}
    echo $PATH
    conda config --set always_yes yes --set changeps1 no
    conda config --set anaconda_upload false --set ssl_verify false
    conda install -n root gcc future
    which python
else
    echo "Mac OS"
    OS=osx-64
fi

conda config --set anaconda_upload no
mkdir ~/conda-bld
if [ `uname` == "Linux" ]; then
    conda install -n root -q anaconda-client "conda-build<3.3"
fi
if [ `uname` == "Darwin" ]; then
    # fix conda and anaconda-client conflict
    conda install conda==4.2.16
    conda install -n root -q anaconda-client conda-build
fi

export CONDA_BLD_PATH=${HOME}/conda-bld
export VERSION=`date +%Y.%m.%d`
export UVCDAT_ANONYMOUS_LOG=no
echo "Cloning recipes"
git clone git://github.com/UV-CDAT/conda-recipes
cd conda-recipes
cmorversion=`echo $1 | cut -d- -f2`
python ./prep_for_build.py -v `date +%Y.%m.%d`.${cmorversion} -b $1
echo "Building now"
#conda build -c conda-forge -c uvcdat/label/nightly -c uvcdat --numpy=1.13 cmor
#conda build -c conda-forge -c uvcdat/label/nightly -c uvcdat --numpy=1.12 cmor
#conda build -c conda-forge -c uvcdat/label/nightly -c uvcdat --numpy=1.11 cmor
#conda build -c conda-forge  -c uvcdat/label/nightly -c uvcdat --numpy=1.10 cmor
conda build -c conda-forge  -c cdat/label/nightly -c cdat cmor --python=${CMOR_PYTHON_VERSION}
mkdir -p ~/.continuum/anaconda-client/
echo "ssl_verify: false" >> ~/.continuum/anaconda-client/config.yaml
echo "verify_ssl: false" >> ~/.continuum/anaconda-client/config.yaml
if [ `uname` == "Darwin" ]; then
    # fix conda and anaconda-client conflict
    conda install conda==4.2.16
fi
anaconda -t $CONDA_UPLOAD_TOKEN upload -u $USER -l nightly $CONDA_BLD_PATH/$OS/$PKG_NAME-`date +%Y.%m.%d`.${cmorversion}*_0.tar.bz2 --force

