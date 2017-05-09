# Only load the deps modules while we're loading deps

set(old_module_path ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH
    ${cmor_SOURCE_DIR}/CMake/deps
    ${CMAKE_MODULE_PATH}
)

file(GLOB dependencies 
     "${cmor_SOURCE_DIR}/CMake/deps/*_deps.cmake"
)

set(cmor_EXTERNALS ${CMAKE_INSTALL_PREFIX}/Externals)
set(ENV{PATH} "${cmor_EXTERNALS}/bin:$ENV{PATH}")
set(CMOR_PACKAGE_CACHE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}"
    CACHE PATH
    "Directory where source tar balls of external dependencies are kept"
)

set(cmor_CMAKE_SOURCE_DIR ${cmor_SOURCE_DIR}/CMake)
set(cmor_CMAKE_BINARY_DIR ${cmor_BINARY_DIR}/CMake)
configure_file(${cmor_CMAKE_SOURCE_DIR}/extra/cmor_common_environment.cmake.in ${cmor_CMAKE_BINARY_DIR}/cmor_common_environment.cmake @ONLY)
configure_file(${cmor_CMAKE_SOURCE_DIR}/extra/cmor_configure_step.cmake.in ${cmor_CMAKE_BINARY_DIR}/cmor_configure_step.cmake @ONLY)

set(deps "")
set(dependencies "${cmor_SOURCE_DIR}/CMake/deps/wget_deps.cmake;")
foreach(dep ${dependencies})
  get_filename_component(fname ${dep} NAME_WE)
  string(REPLACE "_" ";" dep_name ${fname})
  list(GET dep_name 0 pkg)
  list(APPEND deps ${pkg})
  include(${pkg}_pkg)
  # Load all dependency lists
  include(${dep})
  include(${pkg}_external)
endforeach()

# Reset the module path so nothing else finds the deps modules
set(CMAKE_MODULE_PATH ${old_module_path})
