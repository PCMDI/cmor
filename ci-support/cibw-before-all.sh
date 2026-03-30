#!/usr/bin/env bash

set -euo pipefail

CMOR_DEPS_PREFIX=${CMOR_DEPS_PREFIX:?CMOR_DEPS_PREFIX must be set}

download_file() {
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

linux_system_libdir() {
    if [ -d /usr/lib64 ]; then
        echo /usr/lib64
    else
        echo /usr/lib
    fi
}

link_linux_system_deps() {
    local system_libdir

    system_libdir=$(linux_system_libdir)

    mkdir -p "${CMOR_DEPS_PREFIX}/include" "${CMOR_DEPS_PREFIX}/lib" "${CMOR_DEPS_PREFIX}/share"

    [ -d /usr/include/json-c ] && ln -sfn /usr/include/json-c "${CMOR_DEPS_PREFIX}/include/json-c"
    [ -d /usr/include/uuid ] && ln -sfn /usr/include/uuid "${CMOR_DEPS_PREFIX}/include/uuid"
    [ -d /usr/include/udunits2 ] && ln -sfn /usr/include/udunits2 "${CMOR_DEPS_PREFIX}/include/udunits2"
    [ -f /usr/include/udunits2.h ] && ln -sfn /usr/include/udunits2.h "${CMOR_DEPS_PREFIX}/include/udunits2.h"
    [ -d /usr/share/udunits ] && ln -sfn /usr/share/udunits "${CMOR_DEPS_PREFIX}/share/udunits"

    shopt -s nullglob
    for lib_path in \
        "${system_libdir}"/libjson-c.so* \
        "${system_libdir}"/libudunits2.so* \
        "${system_libdir}"/libuuid.so*
    do
        ln -sfn "${lib_path}" "${CMOR_DEPS_PREFIX}/lib/$(basename "${lib_path}")"
    done
    shopt -u nullglob
}

install_linux_build_deps() {
    if command -v dnf >/dev/null 2>&1; then
        dnf install -y epel-release || true
        dnf install -y dnf-plugins-core || true
        dnf config-manager --set-enabled crb || true
        dnf install -y \
            curl \
            gcc \
            gcc-c++ \
            make \
            binutils \
            glibc-devel \
            glibc-headers \
            kernel-headers \
            libstdc++-devel \
            pkgconf-pkg-config \
            zlib-devel \
            hdf5-devel \
            json-c-devel \
            udunits2-devel
        dnf install -y libuuid-devel || dnf install -y util-linux-devel
        return
    fi

    if command -v yum >/dev/null 2>&1; then
        yum install -y epel-release || true
        yum install -y \
            curl \
            gcc \
            gcc-c++ \
            make \
            binutils \
            glibc-devel \
            glibc-headers \
            kernel-headers \
            libstdc++-devel \
            pkgconfig \
            zlib-devel \
            hdf5-devel \
            json-c-devel \
            udunits2-devel
        yum install -y libuuid-devel || yum install -y util-linux-devel
        return
    fi

    echo "Could not find dnf or yum to install Linux build dependencies" >&2
    exit 1
}

build_linux_netcdf_c() {
    local netcdf_c_version
    local netcdf_tarball
    local netcdf_url
    local netcdf_archive_path
    local netcdf_source_dir
    local cppflags
    local ldflags

    netcdf_c_version=${NETCDF_C_VERSION:-4.9.2}
    netcdf_tarball="v${netcdf_c_version}.tar.gz"
    netcdf_url="https://github.com/Unidata/netcdf-c/archive/refs/tags/${netcdf_tarball}"
    netcdf_archive_path="/tmp/netcdf-c-${netcdf_c_version}.tar.gz"
    netcdf_source_dir="/tmp/netcdf-c-${netcdf_c_version}"
    cppflags=""
    ldflags=""

    if [ -x "${CMOR_DEPS_PREFIX}/bin/nc-config" ]; then
        return
    fi

    rm -rf "${CMOR_DEPS_PREFIX}" "${netcdf_source_dir}"
    mkdir -p "${CMOR_DEPS_PREFIX}"

    download_file "${netcdf_url}" "${netcdf_archive_path}"
    tar -C /tmp -xf "${netcdf_archive_path}"

    if [ -d /usr/include/hdf5/serial ]; then
        cppflags="${cppflags} -I/usr/include/hdf5/serial"
    fi

    if [ -d /usr/lib64/hdf5/serial ]; then
        ldflags="${ldflags} -L/usr/lib64/hdf5/serial"
    elif [ -d /usr/lib/hdf5/serial ]; then
        ldflags="${ldflags} -L/usr/lib/hdf5/serial"
    fi

    (
        cd "${netcdf_source_dir}"
        CPPFLAGS="${cppflags}" \
        LDFLAGS="${ldflags}" \
        ./configure \
            --prefix="${CMOR_DEPS_PREFIX}" \
            --disable-dap \
            --disable-byterange \
            --disable-libxml2 \
            --enable-netcdf-4
        make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
        make install
    )

    link_linux_system_deps
}

setup_macos_miniforge() {
    local miniforge_url

    if [ -x "${CMOR_DEPS_PREFIX}/bin/mamba" ]; then
        return
    fi

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

    download_file "${miniforge_url}" /tmp/miniforge.sh
    bash /tmp/miniforge.sh -b -p "${CMOR_DEPS_PREFIX}"
}

install_macos_build_deps() {
    setup_macos_miniforge
    "${CMOR_DEPS_PREFIX}/bin/mamba" install -y -n base -c conda-forge \
        json-c \
        udunits2 \
        libnetcdf \
        libuuid
}

case "$(uname -s)" in
    Linux)
        install_linux_build_deps
        build_linux_netcdf_c
        ;;
    Darwin)
        install_macos_build_deps
        ;;
    *)
        echo "Unsupported platform: $(uname -s)" >&2
        exit 1
        ;;
esac
