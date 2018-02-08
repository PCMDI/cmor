#!/usr/bin/env bash
PKG_NAME=cmor
USER=pcmdi
echo "Trying to upload conda"
if [ `uname` == "Linux" ]; then
    OS=linux-64
    echo "Linux OS"
    export PATH="$HOME/miniconda2/bin:$PATH"
    conda update -y -q conda
    conda install -n root -q anaconda-client "conda-build<3.3"
else
    echo "Mac OS"
    OS=osx-64
fi

conda config --set anaconda_upload no
mkdir ~/conda-bld
mkdir -p ~/.continuum/anaconda-client/
echo "ssl_verify: false" >> ~/.continuum/anaconda-client/config.yaml
echo "verify_ssl: false" >> ~/.continuum/anaconda-client/config.yaml
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
conda build -c conda-forge  -c uvcdat/label/nightly -c uvcdat cmor
anaconda -t $CONDA_UPLOAD_TOKEN upload -u $USER -l nightly $CONDA_BLD_PATH/$OS/$PKG_NAME-`date +%Y.%m.%d`.${cmorversion}-*_0.tar.bz2 --force

