## CMOR Quick-Start Installation Guide

This document provides a set of quick-start instructions for compiling and installing a local checkout of CMOR (Climate Model Output Rewriter)
from source. CMOR is typically compiled within a Conda/Mamba environment to easily satisfy its C, Fortran, and Python library dependencies.
It is based on instructions from [PCMDI/cmor3_documentation](https://cmor.llnl.gov/mydoc_cmor3_github/).


### Prerequisites
- `miniforge` installation, as seen in the [PCMDI/cmor3_documentation](https://cmor.llnl.gov/mydoc_cmor3_github/#installing-miniforge).
- `mamba` or `conda` executables in your `$PATH`


### 1. Obtain the Source Code and CMIP Tables
First, clone the GitHub repository and initialize the necessary submodules containing the MIP tables:
```bash
git clone --recursive https://github.com/PCMDI/cmor.git
cd cmor
```


### 2. Set Up the Build Environment
It is highly recommended to use [Miniforge](https://github.com/conda-forge/miniforge) (or Miniconda/Anaconda) with `mamba` to install the required
development libraries. Set your compiler variables based on your operating system:

**For Linux:**
```bash
export CONDA_COMPILERS="gcc_linux-64 gfortran_linux-64"
```

**For Apple Silicon Mac (M1/M2/M3):**
```bash
export CONDA_COMPILERS="clang_osx-arm64 gfortran_osx-arm64"
```

**For Intel Mac:**
```bash
export CONDA_COMPILERS="clang_osx-64 gfortran_osx-64"
```

Create and activate the `cmor_dev` environment with all dependencies:
```bash
mamba create -n cmor_dev -c conda-forge \
  libuuid \
  json-c \
  udunits2 \
  hdf5 \
  libnetcdf \
  openblas \
  netcdf4 \
  numpy \
  openssl \
  python \
  pyfive \
  hdf5plugin \
  $CONDA_COMPILERS

mamba activate cmor_dev
```


### 3. Configure the Build
Next, configure the compiler flags and tell the `configure` script where to find the libraries you just downloaded.
Set your shared linking flags based on your operating system:

**For Linux:**
```bash
export LDSHARED_FLAGS="-shared -pthread"
```

**For Mac:**
```bash
export LDSHARED_FLAGS="-bundle -undefined dynamic_lookup"
```

Run the `configure` script, using Python to dynamically find the path to your active Conda environment:
```bash
export PREFIX=$(python -c "import sys; print(sys.prefix)")
./configure \
  --prefix=$PREFIX \
  --with-python \
  --with-uuid=$PREFIX \
  --with-json-c=$PREFIX \
  --with-udunits2=$PREFIX \
  --with-netcdf=$PREFIX \
  --enable-verbose-test
```


### 4. Compile and Install
Once configured successfully, use `make` to build and install the CMOR library and its Python bindings (along with the `PrePARE` tool):
```bash
make install
```


### 5. Test the Installation
To ensure CMOR compiled successfully for C, Fortran, and Python, you can run the test suite. Before running the tests, point the NetCDF
library to the `zstd` filters so it can handle Zstandard compression:
```bash
export HDF5_PLUGIN_PATH="$(python -c 'import hdf5plugin; print(hdf5plugin.PLUGINS_PATH)')"
make test
```

If all tests pass, your local installation of CMOR is ready to use!
