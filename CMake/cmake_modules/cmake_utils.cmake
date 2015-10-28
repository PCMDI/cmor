cmake_minimum_required(VERSION 2.8.8)

# Usage: add_cmor_package(package_name version_string optional default)
#-----------------------------------------------------------------------------
macro (add_cmor_package package_name version_string msg default)
  string(TOUPPER ${package_name} uc_package)
  string(TOLOWER ${package_name} lc_package)
  set(version)
  set(message "Build ${package_name} ${version_string}")
  set(use_system_message "Use system ${package_name}")
  set(option_default ON)
  set(cmor_${package_name}_FOUND OFF)

  # ARGV1 will be the version string
  if(NOT "" STREQUAL "${version_string}")
    set(version "${version_string}")
    message("[INFO] version ${version} of ${uc_package} is required by UVCMOR")
  endif()

  # ARGV2 is the build message
  if(NOT "" STREQUAL "${msg}")
    set(message "${msg}")
  endif()

  # ARGV3 (ON / OFF)
  if(NOT "" STREQUAL "${default}")
    set(option_default ${default})
  endif()

  # Check if package is optional, and if yes populate the GUI appropriately
  option(CMOR_BUILD_${uc_package} "${message}" ${option_default})
  mark_as_advanced(CMOR_BUILD_${uc_package})

  #  If this is an optional package
  if(NOT "" STREQUAL "${default}")
    # Find system package first and if it exits provide an option to use
    # system package
    if(DEFINED version)
      find_package(${package_name} ${version} QUIET)
    else()
      find_package(${package_name} QUIET)
    endif()

    mark_as_advanced(CLEAR CMOR_BUILD_${uc_package})

    if(${package_name}_FOUND OR ${uc_package}_FOUND)
      set(cmor_${package_name}_FOUND ON)
    endif()

    option(CMOR_USE_SYSTEM_${uc_package} "${use_system_message}" OFF)

    # If system package is found and can cmor build package option is OFF
    # then cmor use system package should be ON
    if(cmor_${package_name}_FOUND AND "${option_default}" STREQUAL "SYSTEM")
      set(CMOR_USE_SYSTEM_${uc_package} ON CACHE BOOL "${use_system_message}" FORCE)
    endif()

    # If system package is not found or cmor build package option is ON
    # then cmor use system option should be OFF
    if(NOT cmor_${package_name}_FOUND OR CMOR_BUILD_${uc_package})
      set(CMOR_USE_SYSTEM_${uc_package} OFF CACHE BOOL "${use_system_message}" FORCE)
    endif()

  endif()

  if(NOT cmor_${package_name}_FOUND)
    mark_as_advanced(${package_name}_DIR)
  endif()

  # Check if package is found, if not found or found but user prefers to use cmor package
  # then use cmor package or else use system package
  if(NOT CMOR_USE_SYSTEM_${uc_package})
      list(APPEND external_packages "${package_name}")
      set(${lc_package}_pkg "${package_name}")

  else()

    if(CMOR_USE_SYSTEM_${uc_package})
      message("[INFO] Removing external package ${package_name}")
      unset(${lc_package}_pkg)
      if(external_packages)
        list(REMOVE_ITEM external_packages ${package_name})
      endif()

      if(${uc_package}_INCLUDE_DIR)
        list(APPEND found_system_include_dirs ${${uc_package}_INCLUDE_DIR})
        message("[INFO] Including: ${uc_package}_INCLUDE_DIR: ${${uc_package}_INCLUDE_DIR}")
      endif()

      if(${uc_package}_LIBRARY)
        get_filename_component(lib_path ${${uc_package}_LIBRARY} PATH)
        list(APPEND found_system_libraries ${lib_path})
        message("[INFO]  Linking: ${uc_package}_LIBRARY: ${lib_path}")
      endif()
    endif() # use system package

  endif()
endmacro()

#-----------------------------------------------------------------------------
macro(enable_cmor_package_deps package_name)
  string(TOUPPER ${package_name} uc_package)
  string(TOLOWER ${package_name} lc_package)

  if (CMOR_BUILD_${uc_package})
    foreach(dep ${${package_name}_deps})
      string(TOUPPER ${dep} uc_dep)
      if(NOT CMOR_USE_SYSTEM_${uc_dep} AND NOT CMOR_BUILD_${uc_dep})
        set(CMOR_BUILD_${uc_dep} ON CACHE BOOL "" FORCE)
        # Now make sure that deps of dep is enabled
        enable_cmor_package_deps(${dep})
        message("[INFO] Setting build package -- ${dep} ON -- as required by ${package_name}")
      endif()
      if(NOT DEFINED CMOR_USE_SYSTEM_${uc_dep})
        mark_as_advanced(CMOR_BUILD_${uc_dep})
      endif()
    endforeach()
  endif()
endmacro()

# Disable a cmor package
#-----------------------------------------------------------------------------
macro(disable_cmor_package package_name)
  string(TOUPPER ${package_name} uc_package)
  string(TOLOWER ${package_name} lc_package)

  set(cmor_var CMOR_BUILD_${uc_package})
  if(DEFINED ${cmor_var})
    set_property(CACHE ${cmor_var} PROPERTY VALUE OFF)
  endif()
endmacro()

#
#-----------------------------------------------------------------------------
include(CMakeDependentOption)
macro(add_cmor_package_dependent package_name version build_message value dependencies default)
  string(TOUPPER ${package_name} uc_package)
  string(TOLOWER ${package_name} lc_package)
  set(${lc_package}_pkg "${package_name}")

  cmake_dependent_option(CMOR_BUILD_${uc_package} "${message}" ${value} "${dependencies}" ${default})

  # We need this internal variable so that we can diffrentiate between the
  # the case where use has chosen to turn off this packge vs when the packge is
  # evaluated to turn off by cmake dependent option
  cmake_dependent_option(cmor_build_internal_${uc_package} "${message}" ${value} "${dependencies}" ${default})
  set(CACHE cmor_build_internal_${uc_package} PROPERTY TYPE INTERNAL)

#  if (cmor_build_internal_${uc_package})
    add_cmor_package("${package_name}" "${version}" "${build_message}" ${CMOR_BUILD_${uc_package}})
#  else()
    if (DEFINED CMOR_USE_SYSTEM_${uc_package})
      set_property(CACHE CMOR_USE_SYSTEM_${uc_package} PROPERTY TYPE INTERNAL)
      set_property(CACHE CMOR_USE_SYSTEM_${uc_package} PROPERTY VALUE OFF)
    endif()
#  endif()

endmacro()
