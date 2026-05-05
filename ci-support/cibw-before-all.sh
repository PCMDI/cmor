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

sha256_file() {
    local file_path=$1

    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "${file_path}" | awk '{print $1}'
        return
    fi

    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "${file_path}" | awk '{print $1}'
        return
    fi

    echo "Could not find sha256sum or shasum to verify ${file_path}" >&2
    exit 1
}

verify_sha256() {
    local file_path=$1
    local expected_sha256=$2
    local actual_sha256

    actual_sha256=$(sha256_file "${file_path}")
    if [ "${actual_sha256}" != "${expected_sha256}" ]; then
        echo "SHA-256 mismatch for ${file_path}" >&2
        echo "Expected: ${expected_sha256}" >&2
        echo "Actual:   ${actual_sha256}" >&2
        exit 1
    fi
}

netcdf_c_sha256_for_version() {
    local netcdf_c_version=$1

    case "${netcdf_c_version}" in
        4.10.0)
            echo "bd4bfa239385802a6cbea71a2f038dadcaba756a38c56804f829ed8262e7912f"
            ;;
        *)
            return 1
            ;;
    esac
}

find_udunits_xml() {
    local search_root=$1

    find -L "${search_root}" -type f -name 'udunits2*.xml' 2>/dev/null | sort | head -n 1
}

stage_udunits_xml_in_prefix() {
    local xml_source=$1
    local staged_dir="${CMOR_DEPS_PREFIX}/share/cmor"

    mkdir -p "${staged_dir}"
    find -L "$(dirname "${xml_source}")" -maxdepth 1 -type f -name 'udunits2*.xml' -exec cp {} "${staged_dir}/" \;
}

ensure_staged_udunits_xml_in_prefix() {
    local udunits_xml_path=""

    udunits_xml_path=$(find_udunits_xml "${CMOR_DEPS_PREFIX}")
    if [ -z "${udunits_xml_path}" ]; then
        echo "Could not find udunits2 XML data under ${CMOR_DEPS_PREFIX}" >&2
        exit 1
    fi

    stage_udunits_xml_in_prefix "${udunits_xml_path}"

    if [ ! -f "${CMOR_DEPS_PREFIX}/share/cmor/udunits2.xml" ]; then
        echo "Failed to stage udunits2 XML data under ${CMOR_DEPS_PREFIX}/share/cmor" >&2
        exit 1
    fi
}

run_macos_prefix_mamba() {
    env \
        -u CONDA_DEFAULT_ENV \
        -u CONDA_EXE \
        -u CONDA_PREFIX \
        -u CONDA_PROMPT_MODIFIER \
        -u CONDA_PYTHON_EXE \
        -u CONDA_SHLVL \
        -u MAMBA_EXE \
        -u MAMBA_ROOT_PREFIX \
        -u _CE_CONDA \
        -u _CE_M \
        "${CMOR_DEPS_PREFIX}/bin/mamba" "$@"
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
        "${system_libdir}"/libzstd.so* \
        "${system_libdir}"/libuuid.so*
    do
        ln -sfn "${lib_path}" "${CMOR_DEPS_PREFIX}/lib/$(basename "${lib_path}")"
    done
    shopt -u nullglob
}

append_pkg_config_package() {
    local package_name=$1
    local package_cflags
    local package_ldflags
    local package_libs

    if ! pkg-config --exists "${package_name}"; then
        return 1
    fi

    package_cflags=$(pkg-config --cflags "${package_name}")
    package_ldflags=$(pkg-config --libs-only-L "${package_name}")
    package_libs=$(pkg-config --libs-only-l "${package_name}")

    cppflags="${cppflags}${package_cflags:+ ${package_cflags}}"
    ldflags="${ldflags}${package_ldflags:+ ${package_ldflags}}"
    libs="${libs}${package_libs:+ ${package_libs}}"

    return 0
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
            libzstd-devel \
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
            libzstd-devel \
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
    local netcdf_sha256
    local cppflags
    local ldflags
    local libs
    local make_jobs

    netcdf_c_version=${NETCDF_C_VERSION:-4.10.0}
    netcdf_tarball="netcdf-c-${netcdf_c_version}.tar.gz"
    netcdf_url="https://downloads.unidata.ucar.edu/netcdf-c/${netcdf_c_version}/${netcdf_tarball}"
    netcdf_archive_path="/tmp/netcdf-c-${netcdf_c_version}.tar.gz"
    netcdf_source_dir="/tmp/netcdf-c-${netcdf_c_version}"
    netcdf_sha256=${NETCDF_C_SHA256:-}
    cppflags=""
    ldflags=""
    libs=""

    if [ -z "${netcdf_sha256}" ]; then
        if ! netcdf_sha256=$(netcdf_c_sha256_for_version "${netcdf_c_version}"); then
            echo "No pinned SHA-256 is available for netcdf-c ${netcdf_c_version}; set NETCDF_C_SHA256 explicitly" >&2
            exit 1
        fi
    fi

    if [ -x "${CMOR_DEPS_PREFIX}/bin/nc-config" ]; then
        ensure_staged_udunits_xml_in_prefix
        return
    fi

    rm -rf "${CMOR_DEPS_PREFIX}" "${netcdf_source_dir}"
    mkdir -p "${CMOR_DEPS_PREFIX}"

    download_file "${netcdf_url}" "${netcdf_archive_path}"
    verify_sha256 "${netcdf_archive_path}" "${netcdf_sha256}"
    tar -C /tmp -xf "${netcdf_archive_path}"

    if ! append_pkg_config_package hdf5; then
        if [ -d /usr/include/hdf5/serial ]; then
            cppflags="${cppflags} -I/usr/include/hdf5/serial"
        fi

        if [ -d /usr/lib64/hdf5/serial ]; then
            ldflags="${ldflags} -L/usr/lib64/hdf5/serial"
        elif [ -d /usr/lib/hdf5/serial ]; then
            ldflags="${ldflags} -L/usr/lib/hdf5/serial"
        fi
    fi

    append_pkg_config_package hdf5_hl || true

    if ! pkg-config --exists libzstd; then
        echo "Could not find libzstd pkg-config metadata; cannot build zstd-enabled netcdf-c" >&2
        exit 1
    fi

    cppflags="${cppflags} $(pkg-config --cflags libzstd)"
    ldflags="${ldflags} $(pkg-config --libs-only-L libzstd)"
    libs="${libs} $(pkg-config --libs-only-l libzstd)"
    make_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
    if [ "${make_jobs}" -lt 1 ]; then
        make_jobs=1
    fi

    (
        cd "${netcdf_source_dir}"
        if ! CPPFLAGS="${cppflags}" \
            LDFLAGS="${ldflags}" \
            LIBS="${libs}" \
            ./configure \
            --prefix="${CMOR_DEPS_PREFIX}" \
            --disable-dap \
            --disable-byterange \
            --disable-libxml2 \
            --disable-utilities \
            --disable-silent-rules \
            --enable-netcdf-4
        then
            if [ -f config.log ]; then
                echo "===== netcdf-c config.log (tail) =====" >&2
                tail -n 200 config.log >&2 || true
            fi
            exit 1
        fi
        echo "Building netcdf-c ${netcdf_c_version} with make -j${make_jobs}" >&2
        if ! make -j"${make_jobs}"; then
            echo "Parallel netcdf-c build failed; retrying with make -j1 V=1 for clearer diagnostics" >&2
            make -j1 V=1
        fi
        make install
    )

    link_linux_system_deps

    "${CMOR_DEPS_PREFIX}/bin/nc-config" --version

    if ! grep -Rqs "H5Z_FILTER_ZSTD" "${CMOR_DEPS_PREFIX}/include"; then
        echo "Installed netcdf-c does not expose H5Z_FILTER_ZSTD in its headers" >&2
        exit 1
    fi

    if ! grep -Rqs "nc_def_var_zstandard" "${CMOR_DEPS_PREFIX}/include"; then
        echo "Installed netcdf-c does not expose nc_def_var_zstandard in its headers" >&2
        exit 1
    fi

    ensure_staged_udunits_xml_in_prefix
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
    local udunits_xml_path=""

    setup_macos_miniforge
    run_macos_prefix_mamba install -y -p "${CMOR_DEPS_PREFIX}" -c conda-forge \
        json-c \
        udunits2 \
        libnetcdf \
        libuuid

    if [ ! -f "${CMOR_DEPS_PREFIX}/include/udunits2.h" ]; then
        echo "udunits2 headers were not installed under ${CMOR_DEPS_PREFIX}" >&2
        exit 1
    fi

    ensure_staged_udunits_xml_in_prefix
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
