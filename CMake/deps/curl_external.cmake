include(@cmor_CMAKE_BINARY_DIR@/cmor_common_environment.cmake)

set(curl_source "${CMAKE_CURRENT_BINARY_DIR}/build/CURL")
set(curl_install "${cmor_EXTERNALS}")

ExternalProject_Add(CURL
  DOWNLOAD_DIR ${CMOR_PACKAGE_CACHE_DIR}
  SOURCE_DIR ${curl_source}
  INSTALL_DIR ${curl_install}
  URL ${CURL_URL}/${CURL_GZ}
  URL_MD5 ${CURL_MD5}
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DINSTALL_DIR=<INSTALL_DIR> -DWORKING_DIR=<SOURCE_DIR> -P ${cmor_CMAKE_BINARY_DIR}/cmor_configure_step.cmake
  DEPENDS ${CURL_deps}
  ${ep_log_options}
)
