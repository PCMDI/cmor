#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
MAMBA_BIN=${MAMBA_BIN:-mamba}
CONDA_FORGE_CHANNEL=${CONDA_FORGE_CHANNEL:-conda-forge}
RUNNER_ENV_NAME=${RUNNER_ENV_NAME:-cmor-wheel-cibw}
RUNNER_PYTHON_VERSION=${RUNNER_PYTHON_VERSION:-3.11}
UPDATE_SUBMODULES=${UPDATE_SUBMODULES:-1}
WHEEL_DIR=${WHEEL_DIR:-"${ROOT_DIR}/wheelhouse"}
CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX:-/tmp/cmor-miniforge}
MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-11.0}

default_python_versions=(3.10 3.11 3.12 3.13 3.14)

if [ "$#" -gt 0 ]; then
    requested_python_versions=("$@")
else
    requested_python_versions=("${default_python_versions[@]}")
fi

if [ "$(uname -s)" != "Darwin" ]; then
    echo "This script only supports macOS." >&2
    exit 1
fi

if [ "$(uname -m)" != "arm64" ]; then
    echo "This script is intended for Apple Silicon (arm64)." >&2
    exit 1
fi

if ! command -v "${MAMBA_BIN}" >/dev/null 2>&1; then
    echo "Could not find '${MAMBA_BIN}' in PATH." >&2
    exit 1
fi

build_selector_for() {
    case "$1" in
        3.10) echo "cp310-*" ;;
        3.11) echo "cp311-*" ;;
        3.12) echo "cp312-*" ;;
        3.13) echo "cp313-*" ;;
        3.14) echo "cp314-*" ;;
        cp3??-*) echo "$1" ;;
        *)
            echo "Unsupported Python version or build selector: $1" >&2
            exit 1
            ;;
    esac
}

check_python_frameworks() {
    local version
    local missing=()

    for version in "${requested_python_versions[@]}"; do
        case "${version}" in
            3.10|3.11|3.12|3.13|3.14)
                if [ ! -x "/Library/Frameworks/Python.framework/Versions/${version}/bin/python${version}" ]; then
                    missing+=("${version}")
                fi
                ;;
        esac
    done

    if [ "${#missing[@]}" -gt 0 ]; then
        echo "Python.org framework installs not found for: ${missing[*]}" >&2
        echo "Install the requested macOS CPython framework builds from python.org before running local cibuildwheel builds." >&2
        exit 1
    fi
}

ensure_runner_env() {
    if "${MAMBA_BIN}" run -n "${RUNNER_ENV_NAME}" python --version >/dev/null 2>&1; then
        "${MAMBA_BIN}" install -y -n "${RUNNER_ENV_NAME}" -c "${CONDA_FORGE_CHANNEL}" \
            "python=${RUNNER_PYTHON_VERSION}" \
            pip
    else
        "${MAMBA_BIN}" create -y -n "${RUNNER_ENV_NAME}" -c "${CONDA_FORGE_CHANNEL}" \
            "python=${RUNNER_PYTHON_VERSION}" \
            pip
    fi

    "${MAMBA_BIN}" run -n "${RUNNER_ENV_NAME}" python -m pip install --upgrade pip cibuildwheel
}

build_selector=()
for version in "${requested_python_versions[@]}"; do
    build_selector+=("$(build_selector_for "${version}")")
done

check_python_frameworks

if [ "${UPDATE_SUBMODULES}" = "1" ]; then
    git -C "${ROOT_DIR}" submodule update --init
fi

ensure_runner_env

rm -rf "${WHEEL_DIR}"
mkdir -p "${WHEEL_DIR}"

echo "==> Building and testing wheels with cibuildwheel"

(
    cd "${ROOT_DIR}"
    CIBW_ARCHS=arm64 \
    CIBW_BUILD="${build_selector[*]}" \
    CIBW_SKIP="*-musllinux*" \
    CIBW_BEFORE_ALL="bash {project}/ci-support/cibw-before-all.sh" \
    CIBW_BEFORE_BUILD="bash {project}/ci-support/cibw-before-build.sh" \
    CIBW_ENVIRONMENT_MACOS="CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX} CMOR_UDUNITS2_XML=${CMOR_DEPS_PREFIX}/share/cmor/udunits2.xml MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} LDFLAGS='-Wl,-headerpad_max_install_names'" \
    CIBW_REPAIR_WHEEL_COMMAND_MACOS="DYLD_FALLBACK_LIBRARY_PATH=${CMOR_DEPS_PREFIX}/lib delocate-wheel --require-archs {delocate_archs} -w {dest_dir} -v {wheel}" \
    CIBW_TEST_REQUIRES="numpy typing-extensions netcdf4 pyfive hdf5plugin" \
    CIBW_TEST_COMMAND="CMOR_WHEEL_ALREADY_INSTALLED=1 CMOR_TEST_DEPS_ALREADY_INSTALLED=1 bash {project}/ci-support/test-wheel.sh" \
    WHEEL_DIR="${WHEEL_DIR}" \
    "${MAMBA_BIN}" run -n "${RUNNER_ENV_NAME}" python -m cibuildwheel --platform macos --output-dir "${WHEEL_DIR}"
)

ls -lh "${WHEEL_DIR}"/*.whl
