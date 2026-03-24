#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX:?CMOR_DEPS_PREFIX must be set}

cd "${ROOT_DIR}"

export PATH="${CMOR_DEPS_PREFIX}/bin:${PATH}"
export PKG_CONFIG_PATH="${CMOR_DEPS_PREFIX}/lib/pkgconfig:${CMOR_DEPS_PREFIX}/share/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

case "$(uname -s)" in
    Darwin)
        export DYLD_FALLBACK_LIBRARY_PATH="${CMOR_DEPS_PREFIX}/lib${DYLD_FALLBACK_LIBRARY_PATH:+:${DYLD_FALLBACK_LIBRARY_PATH}}"
        ;;
esac

mkdir -p Lib/data
if [ -d "${CMOR_DEPS_PREFIX}/share/udunits" ]; then
    cp "${CMOR_DEPS_PREFIX}"/share/udunits/udunits2*.xml Lib/data/
else
    cp /usr/share/udunits/udunits2*.xml Lib/data/
fi

python_prefix="$(python -c 'import sys; print(sys.prefix)')"

case "$(uname -s)" in
    Linux)
        configure_args=(
            --prefix="${python_prefix}"
            --with-python="${python_prefix}"
            --enable-fortran=no
            --with-uuid="/usr"
            --with-json-c="/usr"
            --with-udunits2="/usr"
            --with-netcdf="${CMOR_DEPS_PREFIX}"
        )
        ;;
    *)
        configure_args=(
            --prefix="${python_prefix}"
            --with-python="${python_prefix}"
            --enable-fortran=no
            --with-uuid="${CMOR_DEPS_PREFIX}"
            --with-json-c="${CMOR_DEPS_PREFIX}"
            --with-udunits2="${CMOR_DEPS_PREFIX}"
            --with-netcdf="${CMOR_DEPS_PREFIX}"
        )
        ;;
esac

if ! ./configure "${configure_args[@]}"; then
    if [ -f config.log ]; then
        echo "===== config.log (tail) =====" >&2
        tail -n 200 config.log >&2 || true
    fi
    exit 77
fi
