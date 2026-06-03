# Fortran examples

## Setup

To run the examples, you will need a Conda or Mamba environment with CMOR, udunits2, and gfortran installed.

```bash
conda create -n cmor-fortran -c conda-forge cmor udunits2 gfortran
conda activate cmor-fortran
```

## Run

To compile and run all of the examples, run the following.

```bash
./run_examples.sh
```

You can also run the examples without activating the environment first.

```bash
conda run -n cmor-fortran ./run_examples.sh
```

The first positional argument sets the output directory.

```bash
./run_examples.sh ./output
```

## Environment variables

The script uses the active Conda environment by default. It expects CMOR under
`$CONDA_PREFIX`, reads the Fortran module from `$CONDA_PREFIX/include`, links
against libraries in `$CONDA_PREFIX/lib`, and prefers a `gfortran` executable
installed in the environment.

Use `CMOR_PREFIX` when CMOR is installed under a different prefix.

```bash
CMOR_PREFIX=/path/to/cmor ./run_examples.sh
```

Use `CMOR_MOD_DIR` or `CMOR_LIB` when the module file or library is not under
the standard `include` and `lib` directories for the prefix.

```bash
CMOR_MOD_DIR=/path/to/include CMOR_LIB=/path/to/libcmor.a ./run_examples.sh
```

Use `FC`, `FFLAGS`, and `EXTRA_LDFLAGS` to override the compiler, compiler
flags, or linker flags.

```bash
FC=/path/to/gfortran \
FFLAGS="-g -O0 -ffree-line-length-none" \
EXTRA_LDFLAGS="-L/path/to/lib -Wl,-rpath,/path/to/lib" \
./run_examples.sh ./output
```

Use `BUILD_DIR` to override where executables and Fortran module outputs are
written.
