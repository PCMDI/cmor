set(netcdf_source "${CMAKE_CURRENT_BINARY_DIR}/build/netcdf")
set(netcdf_install "${cmor_EXTERNALS}")
set(netcdf_configure_args "--enable-netcdf-4")
if (CMOR_BUILD_PARALLEL)
  set(configure_file "cmormpi_configure_step.cmake")
else()
  set(configure_file "cmor_configure_step.cmake")
endif()

configure_file(${cmor_CMAKE_SOURCE_DIR}/cmor_modules_extra/netcdf_patch_step.cmake.in
  ${cmor_CMAKE_BINARY_DIR}/netcdf_patch_step.cmake
  @ONLY)
  
set(netcdf_PATCH_COMMAND ${CMAKE_COMMAND} -P ${cmor_CMAKE_BINARY_DIR}/netcdf_patch_step.cmake)

ExternalProject_Add(NetCDF
  LIST_SEPARATOR ^^
  DOWNLOAD_DIR ${CMOR_PACKAGE_CACHE_DIR}
  SOURCE_DIR ${netcdf_source}
  INSTALL_DIR ${netcdf_install}
  URL ${NC4_URL}/${NC4_GZ}
  URL_MD5 ${NC4_MD5}
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ${netcdf_PATCH_COMMAND}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DINSTALL_DIR=<INSTALL_DIR> -DWORKING_DIR=<SOURCE_DIR> -DCONFIGURE_ARGS=${netcdf_configure_args} -P ${cmor_CMAKE_BINARY_DIR}/${configure_file}
  BUILD_COMMAND ${CMAKE_COMMAND} -Dmake=$(MAKE) -DWORKING_DIR=<SOURCE_DIR> -P ${cmor_CMAKE_BINARY_DIR}/cmor_make_step.cmake
  INSTALL_COMMAND ${CMAKE_COMMAND} -DWORKING_DIR=<SOURCE_DIR> -P ${cmor_CMAKE_BINARY_DIR}/cmor_install_step.cmake
  DEPENDS ${NetCDF_deps}
  ${ep_log_options}
)

