set(wget_source "${CMAKE_CURRENT_BINARY_DIR}/build/wget")
set(wget_install "${cmor_EXTERNALS}")

ExternalProject_Add(Wget
  DOWNLOAD_DIR ${CMOR_PACKAGE_CACHE_DIR}
  SOURCE_DIR ${wget_source}
  INSTALL_DIR ${wget_install}
  URL ${WGET_URL}/${WGET_GZ}
  URL_MD5 ${WGET_MD5}
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DINSTALL_DIR=<INSTALL_DIR> -DWORKING_DIR=<SOURCE_DIR> -P ${cmor_CMAKE_BINARY_DIR}/cmor_configure_step.cmake
  DEPENDS ${wget_deps}
  ${ep_log_options}
)

