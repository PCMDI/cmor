#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_ROOT="${1:-$SCRIPT_DIR/output}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"

if [[ -z "${CONDA_PREFIX:-}" ]]; then
  echo "Activate your conda environment or run: conda run -n <env-name> $0" >&2
  exit 2
fi

detect_fortran_compiler() {
  if [[ -n "${FC:-}" ]]; then
    command -v "$FC" >/dev/null 2>&1 || {
      echo "FC is set to '$FC', but that compiler was not found" >&2
      exit 2
    }
    echo "$FC"
    return
  fi

  local compiler
  for compiler in "$CONDA_PREFIX"/bin/*gfortran "$CONDA_PREFIX"/bin/gfortran; do
    if [[ -x "$compiler" ]]; then
      echo "$compiler"
      return
    fi
  done

  if command -v gfortran >/dev/null 2>&1; then
    command -v gfortran
    return
  fi

  echo "Could not find a Fortran compiler. Install gfortran in the active conda environment or set FC." >&2
  exit 2
}

mkdir -p "$BUILD_DIR" "$OUTPUT_ROOT"

FC="$(detect_fortran_compiler)"
CMOR_LIB="${CMOR_LIB:-$REPO_ROOT/libcmor.a}"
CMOR_MOD_DIR="${CMOR_MOD_DIR:-$REPO_ROOT}"

if [[ ! -f "$CMOR_LIB" ]]; then
  echo "Could not find $CMOR_LIB. Build CMOR first or set CMOR_LIB." >&2
  exit 2
fi

if [[ ! -f "$CMOR_MOD_DIR/cmor_users_functions.mod" ]]; then
  echo "Could not find cmor_users_functions.mod in $CMOR_MOD_DIR. Build CMOR first or set CMOR_MOD_DIR." >&2
  exit 2
fi

FFLAGS_DEFAULT="-g -O2 -ffree-line-length-none"
FFLAGS="${FFLAGS:-$FFLAGS_DEFAULT}"
EXTRA_LDFLAGS="${EXTRA_LDFLAGS:-}"
LINK_FLAGS="$CMOR_LIB -L$CONDA_PREFIX/lib -lnetcdf -ludunits2 -ljson-c -luuid -lm -Wl,-rpath,$CONDA_PREFIX/lib $EXTRA_LDFLAGS"
INCLUDES="-I$CMOR_MOD_DIR -I$REPO_ROOT/include -I$BUILD_DIR -I$CONDA_PREFIX/include"
COMMON_SRC="$SCRIPT_DIR/cmip7_fortran_common.f90"

examples=(
  example_01_regular_grid_tos
  example_02_pressure_levels
  example_03_scalar_height_tas
  example_04_basin_axis
  example_05_hybrid_sigma_levels
  example_06_curvilinear_grid
  example_07_fixed_field_rootd
)

for example in "${examples[@]}"; do
  src="$SCRIPT_DIR/$example.f90"
  exe="$BUILD_DIR/$example"
  run_output="$OUTPUT_ROOT/$example"
  mkdir -p "$run_output"

  echo "Compiling $example"
  "$FC" $FFLAGS -J "$BUILD_DIR" $INCLUDES "$COMMON_SRC" "$src" $LINK_FLAGS -o "$exe"

  echo "Running $example"
  "$exe" "$REPO_ROOT" "$run_output"
done

echo "Wrote CMIP7 example output under $OUTPUT_ROOT"
