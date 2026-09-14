#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TABLES_PATH="${CMOR_TABLES_PATH:-$REPO_ROOT/cmip7-cmor-tables/tables}"
OUTPUT_ROOT="${1:-$SCRIPT_DIR/output}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"

cd "$SCRIPT_DIR"

if [[ -z "${CONDA_PREFIX:-}" ]]; then
  echo "Activate your conda environment or run: conda run -n <env-name> $0" >&2
  exit 2
fi

detect_c_compiler() {
  if [[ -n "${CC:-}" ]]; then
    command -v "$CC" >/dev/null 2>&1 || {
      echo "CC is set to '$CC', but that compiler was not found" >&2
      exit 2
    }
    echo "$CC"
    return
  fi

  local compiler
  for compiler in "$CONDA_PREFIX"/bin/*-cc "$CONDA_PREFIX"/bin/*-gcc "$CONDA_PREFIX"/bin/clang "$CONDA_PREFIX"/bin/gcc "$CONDA_PREFIX"/bin/cc; do
    if [[ -x "$compiler" ]]; then
      echo "$compiler"
      return
    fi
  done

  if command -v cc >/dev/null 2>&1; then
    command -v cc
    return
  fi

  echo "Could not find a C compiler. Install c-compiler in the active conda environment or set CC." >&2
  exit 2
}

mkdir -p "$BUILD_DIR" "$OUTPUT_ROOT"

if [[ ! -d "$TABLES_PATH" ]]; then
  echo "Could not find CMIP7 tables under $TABLES_PATH. Clone cmip7-cmor-tables or set CMOR_TABLES_PATH." >&2
  exit 2
fi

CC="$(detect_c_compiler)"
CMOR_PREFIX="${CMOR_PREFIX:-$CONDA_PREFIX}"
CMOR_INCLUDE_DIR="${CMOR_INCLUDE_DIR:-$CMOR_PREFIX/include}"
CMOR_CDTIME_INCLUDE_DIR="${CMOR_CDTIME_INCLUDE_DIR:-$CMOR_INCLUDE_DIR/cdTime}"

if [[ -n "${CMOR_LIB:-}" ]]; then
  CMOR_LINK_FLAGS=("$CMOR_LIB")
elif [[ -f "$CMOR_PREFIX/lib/libcmor.a" ]]; then
  CMOR_LINK_FLAGS=("$CMOR_PREFIX/lib/libcmor.a")
elif [[ -f "$CMOR_PREFIX/lib/libcmor.dylib" || -f "$CMOR_PREFIX/lib/libcmor.so" ]]; then
  CMOR_LINK_FLAGS=("-L$CMOR_PREFIX/lib" "-lcmor")
else
  echo "Could not find CMOR in $CMOR_PREFIX. Install cmor in the active conda environment or set CMOR_PREFIX/CMOR_LIB." >&2
  exit 2
fi

if [[ ! -f "$CMOR_INCLUDE_DIR/cmor.h" ]]; then
  echo "Could not find cmor.h in $CMOR_INCLUDE_DIR. Install cmor in the active conda environment or set CMOR_INCLUDE_DIR." >&2
  exit 2
fi
if [[ ! -f "$CMOR_INCLUDE_DIR/cdmsint.h" && ! -f "$CMOR_CDTIME_INCLUDE_DIR/cdmsint.h" ]]; then
  echo "Could not find cdmsint.h in $CMOR_INCLUDE_DIR or $CMOR_CDTIME_INCLUDE_DIR. Install cmor in the active conda environment or set CMOR_CDTIME_INCLUDE_DIR." >&2
  exit 2
fi

CFLAGS_DEFAULT="-g -O2 -Wall -Wextra"
CFLAGS="${CFLAGS:-$CFLAGS_DEFAULT}"
EXTRA_LDFLAGS="${EXTRA_LDFLAGS:-}"
LINK_DIRS=("$CMOR_PREFIX/lib")
if [[ "$CONDA_PREFIX/lib" != "$CMOR_PREFIX/lib" ]]; then
  LINK_DIRS+=("$CONDA_PREFIX/lib")
fi

LINK_FLAGS=("${CMOR_LINK_FLAGS[@]}")
for link_dir in "${LINK_DIRS[@]}"; do
  LINK_FLAGS+=("-L$link_dir")
done
LINK_FLAGS+=("-lnetcdf" "-ludunits2" "-ljson-c" "-luuid" "-lm")
for link_dir in "${LINK_DIRS[@]}"; do
  LINK_FLAGS+=("-Wl,-rpath,$link_dir")
done

INCLUDES=("-I$SCRIPT_DIR" "-I$CMOR_INCLUDE_DIR" "-I$CMOR_CDTIME_INCLUDE_DIR" "-I$CONDA_PREFIX/include")
COMMON_SRC="$SCRIPT_DIR/cmip7_c_common.c"

examples=(
  example_01_regular_grid_tos
  example_02_pressure_levels
  example_03_scalar_height_tas
  example_04_basin_axis
  example_05_hybrid_sigma_levels
  example_06_curvilinear_grid
  example_07_fixed_field_rootd
  example_08_site_dimension
)

for example in "${examples[@]}"; do
  src="$SCRIPT_DIR/$example.c"
  exe="$BUILD_DIR/$example"
  run_output="$OUTPUT_ROOT/$example"
  mkdir -p "$run_output"

  echo "Compiling $example"
  "$CC" $CFLAGS "${INCLUDES[@]}" "$COMMON_SRC" "$src" "${LINK_FLAGS[@]}" $EXTRA_LDFLAGS -o "$exe"

  echo "Running $example"
  "$exe" "$REPO_ROOT" "$run_output"
done

echo "Wrote CMIP7 example output under $run_output"
