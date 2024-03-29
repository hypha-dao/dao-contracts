cmake_minimum_required(VERSION 3.16)

# Bypass compiler check - normal c++ can't handle eosio-cpp flags and fails
set(CMAKE_CXX_COMPILER_WORKS 1)

project(dao)

include(ExternalProject)

# if no cdt root is given use default path
if(EOSIO_CDT_ROOT STREQUAL "" OR NOT EOSIO_CDT_ROOT)
   find_package(eosio.cdt)
endif()

ExternalProject_Add(
   dao_project
   SOURCE_DIR ${CMAKE_SOURCE_DIR}/src
   BINARY_DIR ${CMAKE_BINARY_DIR}/dao
   CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake
   UPDATE_COMMAND ""
   PATCH_COMMAND ""
   TEST_COMMAND ""
   INSTALL_COMMAND ""
   BUILD_ALWAYS 1
)

# Set eosio-cpp flags 
set(CMAKE_CXX_FLAGS "-O=s --lto-opt=O3 --fmerge-all-constants")

