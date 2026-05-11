## Wheels

GitHub Actions wheel builds are defined in `.github/workflows/wheel-build.yml`.

The workflow currently builds wheels for these platform targets:

- Linux `x86_64` on `manylinux_2_28`
- macOS `arm64` on `macos-14`
- macOS `x86_64` on `macos-15-intel`

Across those targets, the workflow builds for CPython `3.10` through `3.14`.

`musllinux` builds are skipped.

## Triggers

The wheel workflow runs on:

- pushes to `main`
- pull requests targeting `main`
- published GitHub releases

Pull requests only build and test wheels.
Pushes to `main` build and test wheels, upload them as short-lived workflow artifacts, and publish a GitHub Pages package index.
Published releases build from the release tag, upload the wheels as workflow artifacts, and then attach them to the GitHub release.

## Build Pipeline

The build uses `cibuildwheel` with:

- `ci-support/cibw-before-all.sh`
- `ci-support/cibw-before-build.sh`
- `ci-support/test-wheel.sh`

Before each platform build:

- Linux installs compiler and library prerequisites inside the manylinux image.
- Linux builds `netcdf-c 4.10.0` from source into the wheel dependency prefix and verifies that the installed headers expose zstandard support.
- macOS bootstraps a Miniforge prefix under `/tmp/cmor-miniforge` when needed and installs `json-c`, `udunits2`, `libnetcdf`, and `libuuid` from `conda-forge`.
- Both platforms stage `udunits2.xml` into `${CMOR_DEPS_PREFIX}/share/cmor/`.

Before each wheel build:

- `./configure` is run against the target Python environment.
- The build is configured with `--enable-fortran=no`.
- `uuid`, `json-c`, `udunits2`, and `netcdf` are taken from `${CMOR_DEPS_PREFIX}`

Wheel repair is platform-specific:

- Linux uses `auditwheel repair --plat manylinux_2_28_x86_64`
- macOS uses `delocate-wheel --require-archs {delocate_archs}`

## Packaging And Tests

The Python package version is read from `configure.ac`.
For pushes to `main`, the workflow appends a local version suffix using the commit SHA: `<release-version>+<git-sha>`.
Release builds keep the plain release version.

The wheel build bundles `udunits2*.xml` into `cmor/data/`.
At import time, `Lib/__init__.py` sets `UDUNITS2_XML_PATH` to the bundled XML when present, otherwise it falls back to `${sys.prefix}/share/udunits/udunits2.xml`.

Wheel tests install these extra test dependencies:

- `numpy`
- `typing-extensions`
- `netcdf4`
- `pyfive`
- `hdf5plugin`

`ci-support/test-wheel.sh` then:

- runs in an isolated test workspace by default
- verifies that `cmor` imports from `site-packages`
- verifies that `UDUNITS2_XML_PATH` is set and points to a real XML file
- exports `HDF5_PLUGIN_PATH` from `hdf5plugin`
- runs `cmor.setup()` against the bundled tables
- executes the curated wheel Python test list in `Test/`

## Published Outputs

Release builds publish wheel files as GitHub release assets.

Pushes to `main` also publish a GitHub Pages wheel index built by `ci-support/build-wheel-pages-index.sh`.
That site includes both a package page and a PEP 503-style simple index, and the landing page advertises:

`pip install cmor --extra-index-url https://pcmdi.github.io/cmor`

## Local Apple Silicon Validation

For local Apple Silicon validation with an existing `mamba` install, run:

`./ci-support/run-wheel-ci-macos-arm64.sh`

You can limit the run to specific interpreters, for example:

`./ci-support/run-wheel-ci-macos-arm64.sh 3.12 3.13`

The script currently supports `3.10` through `3.14`, updates submodules by default, and writes wheels to `./wheelhouse`.
Local macOS `cibuildwheel` runs require the requested CPython framework installs from python.org in `/Library/Frameworks/Python.framework/Versions/`.
