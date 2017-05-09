
set(jpeg_source "${CMAKE_CURRENT_BINARY_DIR}/build/jpeg")
set(jpeg_install "${cmor_EXTERNALS}")

configure_file(${cmor_CMAKE_SOURCE_DIR}/cmor_modules_extra/jpeg_install_step.cmake.in
    ${cmor_CMAKE_BINARY_DIR}/jpeg_install_step.cmake
    @ONLY)

set(jpeg_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${cmor_CMAKE_BINARY_DIR}/jpeg_install_step.cmake)

ExternalProject_Add(jpeg
  DOWNLOAD_DIR ${CMOR_PACKAGE_CACHE_DIR}
  SOURCE_DIR ${jpeg_source}
  INSTALL_DIR ${jpeg_install}
  URL ${JPEG_URL}/${JPEG_GZ}
  URL_MD5 ${JPEG_MD5}
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DINSTALL_DIR=<INSTALL_DIR> -DWORKING_DIR=<SOURCE_DIR> -P ${cmor_CMAKE_BINARY_DIR}/cmor_configure_step.cmake
  INSTALL_COMMAND ${jpeg_INSTALL_COMMAND}
  DEPENDS ${jpeg_deps}
  ${ep_log_options}
)

