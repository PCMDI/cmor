export CFLAGS="-Wall -g -m64 -pipe -O2 -fPIC ${CFLAGS}"
export CXXLAGS="${CFLAGS}"
export CPPFLAGS="-I${PREFIX}/include"
export LDFLAGS="-L${PREFIX}/lib"
CONDA_LST=`conda list`
if [[ ${CONDA_LST}'y' == *'openmpi'* ]]; then
    export CC=mpicc
    export CXX=mpicxx
    export LC_RPATH="${PREFIX}/lib"
    export DYLD_FALLBACK_LIBRARY_PATH=${PREFIX}/lib
fi

if [[ `uname` == "Linux" ]]; then
    export LDSHARED_FLAGS="-shared -pthread"
else
    # For OSX builds, reset the include and linker root path for clang
    export CFLAGS="-Wl,-syslibroot / -isysroot / ${CFLAGS}"
    export LDSHARED_FLAGS="-bundle -undefined dynamic_lookup"
fi

./configure \
    --with-python=${PREFIX}   \
    --with-uuid=${PREFIX} \
    --with-udunits2=${PREFIX} \
    --with-netcdf=${PREFIX} \
    --with-libjson-c=${PREFIX} \
    --prefix=${PREFIX}
make 
make install
# Make sure CMOR UDNITS2 env is still present in the package
ACTIVATE_DIR=$PREFIX/etc/conda/activate.d
DEACTIVATE_DIR=$PREFIX/etc/conda/deactivate.d
mkdir -p $ACTIVATE_DIR
mkdir -p $DEACTIVATE_DIR

cp $RECIPE_DIR/scripts/activate.sh $ACTIVATE_DIR/cmor-activate.sh
cp $RECIPE_DIR/scripts/deactivate.sh $DEACTIVATE_DIR/cmor-deactivate.sh
## END BUILD

