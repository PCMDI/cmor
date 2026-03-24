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
            miniforge_url="https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh"
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
