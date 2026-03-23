#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_ROOT=${BUILD_ROOT:-"${ROOT_DIR}/build/wheel-build"}
STAGE_DIR="${BUILD_ROOT}/source"
WHEEL_DIR=${WHEEL_DIR:-"${ROOT_DIR}/wheelhouse"}
RAW_WHEEL_DIR="${WHEEL_DIR}/raw"
REPAIRED_WHEEL_DIR="${WHEEL_DIR}/repaired"
PYTHON_BIN=${PYTHON_BIN:-python}
DELOCATE_WHEEL_BIN=${DELOCATE_WHEEL_BIN:-delocate-wheel}
AUDITWHEEL_BIN=${AUDITWHEEL_BIN:-auditwheel}

if [ -z "${CONDA_PREFIX:-}" ]; then
    echo "CONDA_PREFIX must be set to build wheels" >&2
    exit 1
fi

UDUNITS_DATA_DIR="${CONDA_PREFIX}/share/udunits"
if [ ! -d "${UDUNITS_DATA_DIR}" ]; then
    echo "Could not find ${UDUNITS_DATA_DIR}" >&2
    exit 1
fi

rm -rf "${BUILD_ROOT}" "${RAW_WHEEL_DIR}" "${REPAIRED_WHEEL_DIR}"
mkdir -p "${STAGE_DIR}" "${RAW_WHEEL_DIR}" "${REPAIRED_WHEEL_DIR}"

rsync -a "${ROOT_DIR}/" "${STAGE_DIR}/" \
    --exclude '.git/' \
    --exclude 'build/' \
    --exclude 'dist/' \
    --exclude 'wheelhouse/' \
    --exclude '__pycache__/' \
    --exclude '.pytest_cache/'

mkdir -p "${STAGE_DIR}/Lib/data"
cp "${UDUNITS_DATA_DIR}"/udunits2*.xml "${STAGE_DIR}/Lib/data/"

pushd "${STAGE_DIR}" >/dev/null

./configure \
    --prefix="${CONDA_PREFIX}" \
    --with-python="${CONDA_PREFIX}" \
    --enable-fortran=no \
    --with-uuid="${CONDA_PREFIX}" \
    --with-json-c="${CONDA_PREFIX}" \
    --with-udunits2="${CONDA_PREFIX}" \
    --with-netcdf="${CONDA_PREFIX}"

"${PYTHON_BIN}" -m pip wheel --no-build-isolation --no-deps --wheel-dir "${RAW_WHEEL_DIR}" .

case "$(uname -s)" in
    Darwin)
        "${DELOCATE_WHEEL_BIN}" -w "${REPAIRED_WHEEL_DIR}" -v "${RAW_WHEEL_DIR}"/*.whl
        ;;
    Linux)
        "${AUDITWHEEL_BIN}" repair -w "${REPAIRED_WHEEL_DIR}" "${RAW_WHEEL_DIR}"/*.whl
        ;;
    *)
        echo "Unsupported platform for wheel repair: $(uname -s)" >&2
        exit 1
        ;;
esac

popd >/dev/null

mv "${REPAIRED_WHEEL_DIR}"/*.whl "${WHEEL_DIR}/"
rm -rf "${RAW_WHEEL_DIR}" "${REPAIRED_WHEEL_DIR}"

ls -lh "${WHEEL_DIR}"
