# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CMOR (Climate Model Output Rewriter) is a C library with Fortran and Python interfaces that writes climate model output to NetCDF files following CMIP (Coupled Model Intercomparison Project) conventions. It validates output against JSON table definitions and ensures compliance with CF conventions and CMIP standards.

## Build System

### Recommended: Build with Mamba

The recommended approach is to use Miniforge/mamba to manage dependencies. This ensures all required libraries are properly configured.

1. **Install Miniforge** for your operating system from https://github.com/conda-forge/miniforge

2. **Create a development environment** with all dependencies:

```bash
# Linux
mamba create -n cmor_dev -c conda-forge libuuid json-c udunits2 hdf5 \
  libnetcdf openblas netcdf4 numpy openssl python pyfive typing-extensions \
  gcc_linux-64 gfortran_linux-64

# macOS on x86-64
mamba create -n cmor_dev -c conda-forge libuuid json-c udunits2 hdf5 \
  libnetcdf openblas netcdf4 numpy openssl python pyfive typing-extensions \
  clang_osx-64 gfortran_osx-64

# macOS on Apple Silicon
mamba create -n cmor_dev -c conda-forge libuuid json-c udunits2 hdf5 \
  libnetcdf openblas netcdf4 numpy openssl python pyfive typing-extensions \
  clang_osx-arm64 gfortran_osx-arm64
```

3. **Activate the environment**:

```bash
mamba activate cmor_dev
```

4. **Set environment variables**:

```bash
# Linux
export LDSHARED_FLAGS="-shared -pthread"

# macOS
export LDSHARED_FLAGS=" -bundle -undefined dynamic_lookup"

# Both platforms
export PREFIX=$(python -c "import sys; print(sys.prefix)")
```

5. **Configure CMOR**:

```bash
./configure --prefix=$PREFIX --with-python --with-uuid=$PREFIX \
  --with-json-c=$PREFIX --with-udunits2=$PREFIX --with-netcdf=$PREFIX \
  --enable-verbose-test
```

6. **Build and install**:

```bash
make install
```

7. **Run tests**:

```bash
make test
```

### Alternative: Manual Build with Autotools

If building without mamba, dependencies must be specified if installed in non-standard locations:

```bash
./configure \
  --prefix=/install/path \
  --with-netcdf=/path/to/netcdf \
  --with-udunits2=/path/to/udunits2 \
  --with-json-c=/path/to/json-c \
  --with-uuid=/path/to/uuid \
  --with-python \
  --enable-fortran

make
make install
```

Use `--disable-fortran` to skip Fortran support if not needed.

### Python Module Only

Build Python module directly (requires numpy and all C dependencies):

```bash
make python
```

Or install with pip:

```bash
python -m pip install .
```

## Testing

### Run All Tests

```bash
make test
```

### Test Individual Components

```bash
make test_C          # C library tests only
make test_python     # Python wrapper tests only
make test_fortran    # Fortran interface tests only
make test_cmip6_cv   # CMIP6 CV validation tests
```

### Run Single Test

For C tests:
```bash
gcc -o test_bin/test_name Test/test_name.c -L. -lcmor $(CFLAGS) $(LDFLAGS)
./test_bin/test_name
```

For Python tests:
```bash
python Test/test_name.py
```

## Directory Structure

- **Src/**: Core C library source files
  - Main CMOR implementation: `cmor.c`, `cmor_axes.c`, `cmor_variables.c`, `cmor_tables.c`, `cmor_grids.c`, `cmor_CV.c`
  - **cdTime/**: Time handling utilities for calendar conversions and arithmetic
  - Fortran interface: `cmor_fortran_interface.f90`, `cmor_cfortran_interface.c`
  - Python C extension: `_cmormodule.c`

- **include/**: Public header files
  - **cmor.h**: Main API definitions, constants, and macros
  - **cmor_func_def.h**: Function prototypes
  - **cdTime/**: Time handling headers

- **Lib/**: Python wrapper implementation
  - **pywrapper.py**: High-level Python API wrapping the C extension
  - **cmor_const.py**: Python constants mirroring C definitions
  - **__init__.py**: Module initialization, sets UDUNITS2_XML_PATH

- **Test/**: Test suite
  - C test programs (test_*.c, ipcc_test_code.c)
  - Python tests (test_python_*.py)
  - JSON configuration examples (CMOR_input_example.json, etc.)

- **TestTables/**: JSON table definitions for testing
  - CMIP6_*.json: CMIP6 table definitions
  - CMIP7_*.json: CMIP7 table definitions

- **cmip6-cmor-tables/**: Git submodule with official CMIP6 tables
- **cmip7-cmor-tables/**: Git submodule with official CMIP7 tables
- **mip-cmor-tables/**: Git submodule with additional MIP tables

- **ci-support/**: CircleCI build and test scripts

## Architecture

### Core Components

The CMOR library consists of several interconnected modules:

1. **Tables Module** ([Src/cmor_tables.c](Src/cmor_tables.c)): Loads and parses JSON table definitions that specify variable metadata, axes definitions, and controlled vocabulary requirements.

2. **Axes Module** ([Src/cmor_axes.c](Src/cmor_axes.c)): Manages coordinate axes (time, lat, lon, depth, etc.), handles coordinate transformations, validates bounds, and manages axis-specific metadata.

3. **Variables Module** ([Src/cmor_variables.c](Src/cmor_variables.c)): Handles variable creation, attribute setting, data writing, and ensures variables conform to table specifications.

4. **Grids Module** ([Src/cmor_grids.c](Src/cmor_grids.c)): Manages complex grid definitions including unstructured grids, grid mappings, and coordinate reference systems.

5. **CV Module** ([Src/cmor_CV.c](Src/cmor_CV.c)): Validates controlled vocabulary (CV) requirements specified in CMIP6_CV.json or CMIP7_CV.json, ensuring dataset attributes meet CMIP requirements.

6. **cdTime** ([Src/cdTime/](Src/cdTime/)): Time handling library supporting multiple calendar types (standard, 360_day, noleap, etc.), time conversions, and arithmetic operations.

### Data Flow

1. **Setup**: `cmor_setup()` initializes the library, sets NetCDF mode, logging
2. **Dataset Configuration**: `cmor_dataset_json()` loads dataset-level metadata from JSON
3. **Table Loading**: `cmor_load_table()` loads variable/axis definitions from JSON tables
4. **Axis Creation**: `cmor_axis()` defines coordinate axes with values and bounds
5. **Grid Definition** (optional): `cmor_grid()` for complex/unstructured grids
6. **Variable Creation**: `cmor_variable()` defines output variables linked to axes
7. **Data Writing**: `cmor_write()` writes data, performs validation, applies compression
8. **Cleanup**: `cmor_close()` finalizes files and releases resources

### Language Interfaces

- **C API**: Direct interface defined in [include/cmor.h](include/cmor.h)
- **Fortran API**: Wrapper in [Src/cmor_fortran_interface.f90](Src/cmor_fortran_interface.f90), uses iso_c_binding
- **Python API**: Two-layer approach:
  - C extension module ([Src/_cmormodule.c](Src/_cmormodule.c)) provides low-level bindings
  - Python wrapper ([Lib/pywrapper.py](Lib/pywrapper.py)) adds convenience functions, supports cdms2/MV2

### JSON Configuration

CMOR relies heavily on JSON files:

- **Dataset JSON** (e.g., CMOR_input_example.json): Specifies experiment metadata, institution, source, etc.
- **Table JSON** (e.g., CMIP6_Amon.json): Defines variables, axes, grid requirements for a specific MIP table
- **CV JSON** (CMIP6_CV.json): Controlled vocabulary with valid values for experiment_id, institution_id, source_id, etc.

## Dependencies

Required:
- **NetCDF** (NetCDF4 preferred, NetCDF3.6.3 supported): File I/O
- **udunits2**: Unit conversions and validation
- **json-c**: JSON parsing
- **uuid**: Tracking ID generation

Optional:
- **Fortran compiler** (gfortran, ifort): For Fortran interface
- **Python 3.x + numpy**: For Python bindings

## Common Development Workflows

### Adding Support for a New CMIP Table

1. Update git submodules to get latest tables: `git submodule update --remote`
2. Test table loading in [Test/](Test/) directory
3. Validate with test that exercises the new table

### Modifying Core Validation Logic

1. Core validation is in [Src/cmor_CV.c](Src/cmor_CV.c) - handles CV checking
2. Axis validation logic is in [Src/cmor_axes.c](Src/cmor_axes.c)
3. Variable validation is in [Src/cmor_variables.c](Src/cmor_variables.c)
4. After changes, run full test suite: `make test`

### Debugging Test Failures

1. Enable verbose testing: `./configure --enable-verbose-test`
2. Check generated NetCDF files in [Test/](Test/) directory
3. Enable colored output for better error visibility (enabled by default)
4. For Python tests, run directly: `python Test/test_python_name.py`

### Working with Time Handling

Time handling is complex due to multiple calendar systems. Key points:

- cdTime supports: standard (gregorian), noleap (365_day), 360_day, julian, proleptic_gregorian
- Time conversions in [Src/cdTime/cdTimeConv.c](Src/cdTime/cdTimeConv.c)
- Time arithmetic in [Src/cdTime/timeArith.c](Src/cdTime/timeArith.c)
- Python wrapper can accept cdtime objects (from CDAT) or numpy arrays

## CI/CD

CircleCI configuration in [.circleci/config.yml](.circleci/config.yml):
- Builds conda packages for multiple Python versions (3.10-3.14)
- Runs full test suite on Linux
- Uploads to conda-forge channel

## Git Submodules

Initialize/update submodules after cloning:

```bash
git submodule init
git submodule update
```

Or clone with submodules:

```bash
git clone --recursive https://github.com/PCMDI/cmor.git
```
