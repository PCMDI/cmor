#!/usr/bin/env bash

set -euo pipefail

CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX:?CMOR_DEPS_PREFIX must be set}

download_installer() {
    local url=$1
    local output_path=$2

    if command -v curl >/dev/null 2>&1; then
        curl -L "${url}" -o "${output_path}"
        return
    fi

    if command -v wget >/dev/null 2>&1; then
        wget -O "${output_path}" "${url}"
        return
    fi

    echo "Could not find curl or wget to download ${url}" >&2
    exit 1
}

if [ ! -x "${CMOR_DEPS_PREFIX}/bin/mamba" ]; then
    case "$(uname -s)" in
        Linux)
            if command -v dnf >/dev/null 2>&1; then
                dnf install -y epel-release || true
                dnf install -y dnf-plugins-core || true
                if command -v dnf >/dev/null 2>&1; then
                    dnf config-manager --set-enabled crb || true
                fi
                dnf install -y \
                    gcc \
                    gcc-c++ \
                    make \
                    binutils \
                    glibc-devel \
                    json-c-devel \
                    udunits2-devel \
                    netcdf-devel \
                    libuuid-devel
            elif command -v yum >/dev/null 2>&1; then
                yum install -y epel-release || true
                yum install -y \
                    gcc \
                    gcc-c++ \
                    make \
                    binutils \
                    glibc-devel \
                    json-c-devel \
                    udunits2-devel \
                    netcdf-devel \
                    libuuid-devel
            else
                echo "Could not find dnf or yum to install Linux build dependencies" >&2
                exit 1
            fi

            rm -rf "${CMOR_DEPS_PREFIX}"
            mkdir -p "${CMOR_DEPS_PREFIX}/bin" "${CMOR_DEPS_PREFIX}/share"
            ln -sfn /usr/include "${CMOR_DEPS_PREFIX}/include"
            ln -sfn /usr/share/udunits "${CMOR_DEPS_PREFIX}/share/udunits"
            ln -sfn /usr/bin/nc-config "${CMOR_DEPS_PREFIX}/bin/nc-config"
            if [ -d /usr/lib64 ]; then
                ln -sfn /usr/lib64 "${CMOR_DEPS_PREFIX}/lib"
            else
                ln -sfn /usr/lib "${CMOR_DEPS_PREFIX}/lib"
            fi
            exit 0
            ;;
        Darwin)
            case "$(uname -m)" in
                arm64)
                    miniforge_url="https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-arm64.sh"
                    ;;
                x86_64)
                    miniforge_url="https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-x86_64.sh"
                    ;;
                *)
                    echo "Unsupported macOS architecture: $(uname -m)" >&2
                    exit 1
                    ;;
            esac
            ;;
        *)
            echo "Unsupported platform: $(uname -s)" >&2
            exit 1
            ;;
    esac

    download_installer "${miniforge_url}" /tmp/miniforge.sh
    bash /tmp/miniforge.sh -b -p "${CMOR_DEPS_PREFIX}"
fi

export PATH="${CMOR_DEPS_PREFIX}/bin:${PATH}"

"${CMOR_DEPS_PREFIX}/bin/mamba" install -y -n base -c conda-forge \
    json-c \
    udunits2 \
    libnetcdf \
    libuuid
