cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
CMAKE_POLICY(SET CMP0012 NEW)

project(cmor)

set(LLNL_URL http://uv-cdat.llnl.gov/cdat/resources)
set(CMAKE_MODULE_PATH
    ${cmor_SOURCE_DIR}/CMake
    ${cmor_SOURCE_DIR}/CMake/cmake_modules
    ${CMAKE_MODULE_PATH}
)

include(ExternalProject)
include(cmake_utils)
# Loads all of the dependencies
include(deps)

message("[INFO] Building ${external_packages}")

