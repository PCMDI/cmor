#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
MAMBA_BIN=${MAMBA_BIN:-mamba}
CONDA_FORGE_CHANNEL=${CONDA_FORGE_CHANNEL:-conda-forge}
C_COMPILER=${C_COMPILER:-clang_osx-arm64}
ENV_PREFIX=${ENV_PREFIX:-cmor-wheel}
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

ensure_env() {
    local env_name=$1
    shift
    local packages=("$@")

    if "${MAMBA_BIN}" run -n "${env_name}" python --version >/dev/null 2>&1; then
        "${MAMBA_BIN}" install -y -n "${env_name}" -c "${CONDA_FORGE_CHANNEL}" "${packages[@]}"
    else
        "${MAMBA_BIN}" create -y -n "${env_name}" -c "${CONDA_FORGE_CHANNEL}" "${packages[@]}"
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

recreate_test_env() {
    local build_env_name=$1
    local test_env_name=$2

    if "${MAMBA_BIN}" run -n "${test_env_name}" python --version >/dev/null 2>&1; then
        conda env remove -y -n "${test_env_name}"
    fi

    conda create -y -n "${test_env_name}" --clone "${build_env_name}"
}

for python_version in "${PYTHON_VERSIONS[@]}"; do
    build_env_name="${ENV_PREFIX}-build-py${python_version//./}"
    test_env_name="${ENV_PREFIX}-test-py${python_version//./}"
    wheel_dir="${ROOT_DIR}/wheelhouse/py${python_version}"
    build_root="${ROOT_DIR}/build/wheel-build-py${python_version//./}"
    test_venv_dir="${ROOT_DIR}/build/wheel-test-venv-py${python_version//./}"

    echo "==> Preparing ${build_env_name}"
    ensure_env \
        "${build_env_name}" \
        "python=${python_version}" \
        pip \
        setuptools \
        wheel \
        numpy \
        delocate \
        json-c \
        udunits2 \
        libnetcdf \
        libuuid \
        "${C_COMPILER}"

    echo "==> Preparing ${test_env_name}"
    recreate_test_env "${build_env_name}" "${test_env_name}"

    activate_env "${build_env_name}"
    rm -rf "${wheel_dir}" "${build_root}" "${test_venv_dir}"

    echo "==> Building wheel for Python ${python_version}"
    WHEEL_DIR="${wheel_dir}" \
    BUILD_ROOT="${build_root}" \
    bash "${ROOT_DIR}/ci-support/build-wheel.sh"

    deactivate_env
    activate_env "${test_env_name}"

    echo "==> Testing wheel for Python ${python_version}"
    WHEEL_DIR="${wheel_dir}" \
    TEST_VENV_DIR="${test_venv_dir}" \
    bash "${ROOT_DIR}/ci-support/test-wheel.sh"

    deactivate_env
done
