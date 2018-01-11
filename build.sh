#!/bin/bash
#
#
#
# NOTE:  you might need root access to install cmor python egg /usr/local/lib (See last line)
#
# -------------------
# Obtain all packages 
# -------------------
export GIT_SSL_NO_VERIFY=true 
export PREFIX=$HOME/build
# -------------------
# -------------------
mkdir build
cd build
wget http://www.hdfgroup.org/ftp/HDF5/current/src/hdf5-1.8.17.tar
wget ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-4.4.0.tar.gz
wget ftp://ftp.unidata.ucar.edu/pub/udunits/udunits-2.2.20.tar.gz
wget http://www.mirrorservice.org/sites/ftp.ossp.org/pkg/lib/uuid/uuid-1.6.2.tar.gz
#
# -------------------
# Untar packages
# -------------------
tar xf hdf5-1.8.17.tar
tar xzf netcdf-4.4.0.tar.gz
tar xzf udunits-2.2.20.tar.gz
tar xzf uuid-1.6.2.tar.gz
#
# -------------------
# BUILD libuuid
# -------------------
#
cd uuid-1.6.2
./configure --prefix=$PREFIX
make 
make install
#
# -------------------
# build udnits2
# -------------------
#
cd ../udunits-2.2.20
./configure --prefix=$PREFIX
make
make install
#
# -------------------
# build hdf5
# -------------------
#
cd ../hdf5-1.8.17
./configure --prefix=$PREFIX
make  
make install
#
# -------------------
# build netcdf4
# -------------------
#
export CFLAGS="-I$HOME/build/include"
export LDFLAGS="-L$HOME/build/lib"

cd ../netcdf-4.4.0
./configure --prefix=$PREFIX --enable-netcdf4
make 
make install


#
# -------------------
# build cmor
# -------------------
cd ..
git clone https://github.com/PCMDI/cmor.git
cd cmor
git checkout cmor3

./configure --prefix=$PREFIX --with-python=$PREFIX --with-uuid=$PREFIX --with-udunits=$PREFIX --with-netcdf=$PREFIX
make 

#
#  For installing egg without su, temporarily set PYTHONPATH to a directory where you want to 
#  install it, such as e.g. "$HOME/.local/lib/python2.7/site-packages"
#
sudo make install
sudo make python
