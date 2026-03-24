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
            NETCDF_C_VERSION=${NETCDF_C_VERSION:-4.9.2}
            NETCDF_C_TARBALL="v${NETCDF_C_VERSION}.tar.gz"
            NETCDF_C_URL="https://github.com/Unidata/netcdf-c/archive/refs/tags/${NETCDF_C_TARBALL}"
            NETCDF_C_BUILD_ROOT="/tmp/netcdf-c-${NETCDF_C_VERSION}"
            NETCDF_CPPFLAGS=""
            NETCDF_LDFLAGS=""

            if [ -x "${CMOR_DEPS_PREFIX}/bin/nc-config" ]; then
                mkdir -p "${CMOR_DEPS_PREFIX}/share"
                ln -sfn /usr/share/udunits "${CMOR_DEPS_PREFIX}/share/udunits"
                exit 0
            fi

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
                    glibc-headers \
                    kernel-headers \
                    libstdc++-devel \
                    pkgconf-pkg-config \
                    curl-devel \
                    zlib-devel \
                    hdf5-devel \
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
                    glibc-headers \
                    kernel-headers \
                    libstdc++-devel \
                    pkgconfig \
                    curl-devel \
                    zlib-devel \
                    hdf5-devel \
                    json-c-devel \
                    udunits2-devel \
                    netcdf-devel \
                    libuuid-devel
            else
                echo "Could not find dnf or yum to install Linux build dependencies" >&2
                exit 1
            fi

            rm -rf "${CMOR_DEPS_PREFIX}"
            rm -rf "${NETCDF_C_BUILD_ROOT}"
            mkdir -p "${CMOR_DEPS_PREFIX}" "${NETCDF_C_BUILD_ROOT}"

            download_installer "${NETCDF_C_URL}" "/tmp/${NETCDF_C_TARBALL}"
            tar -C /tmp -xf "/tmp/${NETCDF_C_TARBALL}"

            if [ -d /usr/include/hdf5/serial ]; then
                NETCDF_CPPFLAGS="${NETCDF_CPPFLAGS} -I/usr/include/hdf5/serial"
            fi
            if [ -d /usr/lib64/hdf5/serial ]; then
                NETCDF_LDFLAGS="${NETCDF_LDFLAGS} -L/usr/lib64/hdf5/serial"
            elif [ -d /usr/lib/hdf5/serial ]; then
                NETCDF_LDFLAGS="${NETCDF_LDFLAGS} -L/usr/lib/hdf5/serial"
            fi

            cd "${NETCDF_C_BUILD_ROOT}"
            CPPFLAGS="${NETCDF_CPPFLAGS}" \
            LDFLAGS="${NETCDF_LDFLAGS}" \
            ./configure \
                --prefix="${CMOR_DEPS_PREFIX}" \
                --disable-dap \
                --disable-byterange \
                --enable-netcdf-4
            make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
            make install

            mkdir -p "${CMOR_DEPS_PREFIX}/share"
            ln -sfn /usr/share/udunits "${CMOR_DEPS_PREFIX}/share/udunits"
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
