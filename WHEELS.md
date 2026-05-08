## Wheels

GitHub Actions wheel builds are defined in `.github/workflows/wheel-build.yml`.
The workflow uses `cibuildwheel` to build and test Linux, macOS Intel, and Apple Silicon wheels, and then:

- builds Linux wheels inside a manylinux container and macOS wheels against a Miniforge dependency prefix
- builds `netcdf-c` from source on Linux so the wheel uses a manylinux-compatible libnetcdf with quantization and zstandard filter support
- repairs wheels with `auditwheel` on Linux and `delocate` on macOS so they install into standard Python environments
- bundles `udunits2.xml` so `pip install` does not require a local UDUNITS2 data install
- smoke-tests the installed wheel by importing `cmor`, running `cmor.setup()`, and executing the wheel Python test list
- uploads the repaired wheels as assets on the published GitHub release only

The workflow builds and tests wheels on pushes to `main` and pull requests targeting `main`.
When a GitHub release is published, it builds from that release tag and uploads the wheels to the release.

For local Apple Silicon validation with an existing `mamba` install, run:
`./ci-support/run-wheel-ci-macos-arm64.sh`
You can limit the run to specific interpreters, for example:
`./ci-support/run-wheel-ci-macos-arm64.sh 3.12 3.13`
Local macOS `cibuildwheel` runs require the requested CPython framework installs from python.org in `/Library/Frameworks/Python.framework/Versions/`.
