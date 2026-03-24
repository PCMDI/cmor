#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
MAMBA_BIN=${MAMBA_BIN:-mamba}
CONDA_FORGE_CHANNEL=${CONDA_FORGE_CHANNEL:-conda-forge}
RUNNER_ENV_NAME=${RUNNER_ENV_NAME:-cmor-wheel-cibw}
MANUAL_ENV_PREFIX=${MANUAL_ENV_PREFIX:-cmor-wheel}
CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX:-"${ROOT_DIR}/build/cibw-deps-macos-arm64"}
WHEEL_DIR=${WHEEL_DIR:-"${ROOT_DIR}/wheelhouse"}
USE_CIBUILDWHEEL=${USE_CIBUILDWHEEL:-auto}
UPDATE_SUBMODULES=${UPDATE_SUBMODULES:-1}

if [ "$#" -gt 0 ]; then
    PYTHON_VERSIONS=("$@")
else
    PYTHON_VERSIONS=(3.10 3.11 3.12 3.13 3.14)
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

eval "$("${MAMBA_BIN}" shell hook --shell bash)"

if [ "${UPDATE_SUBMODULES}" = "1" ]; then
    git -C "${ROOT_DIR}" submodule update --init
fi

ensure_named_env() {
    local env_name=$1
    shift
    local packages=("$@")

    if "${MAMBA_BIN}" run -n "${env_name}" python --version >/dev/null 2>&1; then
        "${MAMBA_BIN}" install -y -n "${env_name}" -c "${CONDA_FORGE_CHANNEL}" "${packages[@]}"
    else
        "${MAMBA_BIN}" create -y -n "${env_name}" -c "${CONDA_FORGE_CHANNEL}" "${packages[@]}"
    fi
}

ensure_prefix_env() {
    local prefix_path=$1
    shift
    local packages=("$@")

    if [ -x "${prefix_path}/bin/python" ]; then
        "${MAMBA_BIN}" install -y -p "${prefix_path}" -c "${CONDA_FORGE_CHANNEL}" "${packages[@]}"
    else
        "${MAMBA_BIN}" create -y -p "${prefix_path}" -c "${CONDA_FORGE_CHANNEL}" "${packages[@]}"
    fi
}

activate_env() {
    set +u
    mamba activate "$1"
    set -u
}

deactivate_env() {
    set +u
    mamba deactivate
    set -u
}

framework_python_path() {
    local python_version=$1
    echo "/Library/Frameworks/Python.framework/Versions/${python_version}/bin/python${python_version}"
}

has_framework_python() {
    local python_version=$1
    [ -x "$(framework_python_path "${python_version}")" ]
}

python_version_to_selector() {
    case "$1" in
        3.10) echo "cp310-*" ;;
        3.11) echo "cp311-*" ;;
        3.12) echo "cp312-*" ;;
        3.13) echo "cp313-*" ;;
        3.14) echo "cp314-*" ;;
        *)
            echo "Unsupported Python version: $1" >&2
            exit 1
            ;;
    esac
}

echo "==> Preparing native dependency prefix at ${CMOR_DEPS_PREFIX}"
ensure_prefix_env \
    "${CMOR_DEPS_PREFIX}" \
    json-c \
    udunits2 \
    libnetcdf \
    libuuid

mkdir -p "${WHEEL_DIR}"

run_with_cibuildwheel() {
    local build_selector=""
    local python_version

    for python_version in "${PYTHON_VERSIONS[@]}"; do
        build_selector+="$(python_version_to_selector "${python_version}") "
    done
    build_selector=${build_selector%" "}

    echo "==> Preparing ${RUNNER_ENV_NAME}"
    ensure_named_env \
        "${RUNNER_ENV_NAME}" \
        "python=3.11" \
        pip

    activate_env "${RUNNER_ENV_NAME}"

    echo "==> Installing cibuildwheel"
    python -m pip install --upgrade pip
    python -m pip install --upgrade cibuildwheel

    echo "==> Building and testing wheels with cibuildwheel"
    CIBW_ARCHS=arm64 \
    CIBW_BUILD="${build_selector}" \
    CIBW_SKIP="*-musllinux*" \
    CIBW_BEFORE_ALL=":" \
    CIBW_BEFORE_BUILD="bash {project}/ci-support/cibw-before-build.sh" \
    CIBW_ENVIRONMENT="CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX}" \
    CIBW_TEST_REQUIRES="numpy typing-extensions netcdf4 pyfive hdf5plugin" \
    CIBW_TEST_COMMAND="CMOR_WHEEL_ALREADY_INSTALLED=1 bash {project}/ci-support/test-wheel.sh" \
    python -m cibuildwheel --platform macos --output-dir "${WHEEL_DIR}"

    deactivate_env
}

run_manual_build_and_test() {
    local python_version=$1
    local build_env_name="${MANUAL_ENV_PREFIX}-build-py${python_version//./}"
    local test_env_name="${MANUAL_ENV_PREFIX}-test-py${python_version//./}"
    local wheel_file=

    echo "==> Preparing ${build_env_name}"
    ensure_named_env \
        "${build_env_name}" \
        "python=${python_version}" \
        pip \
        setuptools \
        wheel \
        numpy \
        delocate

    echo "==> Preparing ${test_env_name}"
    ensure_named_env \
        "${test_env_name}" \
        "python=${python_version}" \
        pip \
        numpy \
        typing-extensions \
        netcdf4 \
        pyfive \
        hdf5plugin

    rm -rf "${ROOT_DIR}/dist"

    activate_env "${build_env_name}"

    echo "==> Building wheel for Python ${python_version} with local fallback"
    python -m pip install --upgrade pip
    python -m pip install --upgrade build
    MACOSX_DEPLOYMENT_TARGET=11.0 \
    CMOR_DEPS_PREFIX="${CMOR_DEPS_PREFIX}" \
    bash "${ROOT_DIR}/ci-support/cibw-before-build.sh"
    MACOSX_DEPLOYMENT_TARGET=11.0 python -m build --wheel --no-isolation
    delocate-wheel -w "${WHEEL_DIR}" -v "${ROOT_DIR}/dist/"*.whl
    wheel_file=$(ls -t "${WHEEL_DIR}"/*.whl | head -n 1)

    deactivate_env
    activate_env "${test_env_name}"

    echo "==> Testing wheel for Python ${python_version}"
    WHEEL_FILE="${wheel_file}" \
    bash "${ROOT_DIR}/ci-support/test-wheel.sh"

    deactivate_env
}

requested_versions_missing_framework=()
for python_version in "${PYTHON_VERSIONS[@]}"; do
    if ! has_framework_python "${python_version}"; then
        requested_versions_missing_framework+=("${python_version}")
    fi
done

case "${USE_CIBUILDWHEEL}" in
    1|true|yes)
        if [ "${#requested_versions_missing_framework[@]}" -gt 0 ]; then
            echo "Requested Python.org framework installs are missing for: ${requested_versions_missing_framework[*]}" >&2
            exit 1
        fi
        run_with_cibuildwheel
        ;;
    0|false|no)
        for python_version in "${PYTHON_VERSIONS[@]}"; do
            run_manual_build_and_test "${python_version}"
        done
        ;;
    auto)
        if [ "${#requested_versions_missing_framework[@]}" -eq 0 ]; then
            run_with_cibuildwheel
        else
            echo "==> Python.org framework installs not found for: ${requested_versions_missing_framework[*]}"
            echo "==> Falling back to local build/test flow that reuses the cibuildwheel configure and test scripts"
            for python_version in "${PYTHON_VERSIONS[@]}"; do
                run_manual_build_and_test "${python_version}"
            done
        fi
        ;;
    *)
        echo "Unsupported USE_CIBUILDWHEEL value: ${USE_CIBUILDWHEEL}" >&2
        exit 1
        ;;
esac

ls -lh "${WHEEL_DIR}"
