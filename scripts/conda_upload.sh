#!/usr/bin/env bash
PKG_NAME=cmor
USER=pcmdi
echo "Trying to upload conda"
if [ `uname` == "Linux" ]; then
    OS=linux-64
    echo "Linux OS"
    export PATH="$HOME/miniconda2/bin:$PATH"
    conda update -y -q conda
else
    echo "Mac OS"
    OS=osx-64
fi

mkdir ~/conda-bld
conda install -q anaconda-client conda-build
conda config --set anaconda_upload no
binstar config --set verify_ssl False
export CONDA_BLD_PATH=${HOME}/conda-bld
export VERSION=`date +%Y.%m.%d`
export UVCDAT_ANONYMOUS_LOG=no
echo "Cloning recipes"
git clone git://github.com/UV-CDAT/conda-recipes
cd conda-recipes
cmorversion=`echo $1 | cut -d- -f2`
python ./prep_for_build.py -v `date +%Y.%m.%d`.${cmorversion} -b $1
echo "Building now"
conda build -c conda-forge -c uvcdat/label/nightly -c uvcdat --numpy=1.12 cmor
anaconda -t $CONDA_UPLOAD_TOKEN upload -u $USER -l nightly $CONDA_BLD_PATH/$OS/$PKG_NAME-`date +%Y.%m.%d`.${cmorversion}-np112py27*_0.tar.bz2 --force
conda build -c conda-forge -c uvcdat/label/nightly -c uvcdat --numpy=1.11 cmor
anaconda -t $CONDA_UPLOAD_TOKEN upload -u $USER -l nightly $CONDA_BLD_PATH/$OS/$PKG_NAME-`date +%Y.%m.%d`.${cmorversion}-np111py27*_0.tar.bz2 --force
conda build -c conda-forge  -c uvcdat/label/nightly -c uvcdat --numpy=1.10 cmor
anaconda -t $CONDA_UPLOAD_TOKEN upload -u $USER -l nightly $CONDA_BLD_PATH/$OS/$PKG_NAME-`date +%Y.%m.%d`.${cmorversion}-np110py27*_0.tar.bz2 --force
conda build -c conda-forge  -c uvcdat/label/nightly -c uvcdat --numpy=1.9 cmor
anaconda -t $CONDA_UPLOAD_TOKEN upload -u $USER -l nightly $CONDA_BLD_PATH/$OS/$PKG_NAME-`date +%Y.%m.%d`.${cmorversion}-np19py27*_0.tar.bz2 --force

