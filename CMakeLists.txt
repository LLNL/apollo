# Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
# Apollo project developers. Produced at the Lawrence Livermore National
# Laboratory. See the top-level LICENSE file for details.
# SPDX-License-Identifier: MIT

#
# Apollo
#

project(APOLLO LANGUAGES CXX C)
cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 14)

# ####
#
# Options
#
option(ENABLE_MPI "Require and enable MPI for collective training" OFF)
option(ENABLE_OpenCV "Require and enable OpenCV for ML models" OFF)
option(ENABLE_JIT_DTREE "Enable JIT generated DecisionTree eval functions (supported only by Apollo implementation)" OFF)
option(ENABLE_CUDA "Enable CUDA for asynchronous timing of CUDA regions" OFF)
option(ENABLE_HIP "Enable HIP for asynchronous timing of HIP regions" OFF)
option(ENABLE_TESTS "Enable building Apollo tests" OFF)
option(BUILD_SHARED_LIBS "Build Apollo as a shared library instead of static" OFF)

#
# ####

# Version information
set(APOLLO_MAJOR_VERSION 2)
set(APOLLO_MINOR_VERSION 0)
set(APOLLO_PATCH_VERSION 0)
set(APOLLO_VERSION "${APOLLO_MAJOR_VERSION}.${APOLLO_MINOR_VERSION}.${APOLLO_PATCH_VERSION}")

set(APOLLO_BUILD_TYPE "${CMAKE_BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(PreventInSourceBuilds)

#
# Setup variables to pass to ApolloConfig.h
#
# Pull in macros
include(ApolloProjectMacros)

# Pull in the GIT repo information
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

# add_definitions("-DAPOLLO_GIT_SHA1=${GIT_SHA1}")
set(APOLLO_GIT_SHA1 "${GIT_SHA1}")

# Set some host information
execute_process(COMMAND uname -n OUTPUT_VARIABLE APOLLO_HOST_NODE_NAME)
string(STRIP ${APOLLO_HOST_NODE_NAME} APOLLO_HOST_NODE_NAME)

if(APPLE)
  execute_process(COMMAND uname OUTPUT_VARIABLE APOLLO_HOST_DETAILED_O)
else(APPLE)
  execute_process(COMMAND uname -o OUTPUT_VARIABLE APOLLO_HOST_DETAILED_O)
endif(APPLE)

string(STRIP ${APOLLO_HOST_DETAILED_O} APOLLO_HOST_DETAILED_O)
execute_process(COMMAND uname -r OUTPUT_VARIABLE APOLLO_HOST_DETAILED_R)
string(STRIP ${APOLLO_HOST_DETAILED_R} APOLLO_HOST_DETAILED_R)
execute_process(COMMAND uname -m OUTPUT_VARIABLE APOLLO_HOST_DETAILED_M)
string(STRIP ${APOLLO_HOST_DETAILED_M} APOLLO_HOST_DETAILED_M)
set(APOLLO_HOST_KNOWN_AS ${APOLLO_HOST_NODE_NAME})
set(APOLLO_HOST_DETAILED "\"${APOLLO_HOST_DETAILED_O} ${APOLLO_HOST_DETAILED_R} ${APOLLO_HOST_DETAILED_M}\"")
message(STATUS "Building on: ${CMAKE_SYSTEM_NAME}")

# string(TOUPPER ${APOLLO_BUILD_TYPE} CMAKE_BUILD_TYPE)
set(APOLLO_CXX_COMPILER "${CMAKE_CXX_COMPILER} ")
set(APOLLO_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${APOLLO_BUILD_TYPE}} ")
set(APOLLO_LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_LINK_FLAGS_${CMAKE_BUILD_TYPE}} ")

# Create a config header file
configure_file(
  ${PROJECT_SOURCE_DIR}/apollo-config.h.in
  ${PROJECT_BINARY_DIR}/apollo-config.h)

# Create pkg-confic .pc file
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_LIBS "-L\${libdir} -lapollo ")
set(PKG_CONFIG_CFLAGS "-I\${includedir} ")

configure_file(
  ${PROJECT_SOURCE_DIR}/apollo.pc.in
  ${PROJECT_BINARY_DIR}/apollo.pc)

# Make apollo findable for cmake
configure_file(
  ${PROJECT_SOURCE_DIR}/apollo-config.cmake.in
  ${PROJECT_BINARY_DIR}/apollo-config.cmake
  @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/apollo-config.cmake
  DESTINATION share/cmake/apollo)

install(EXPORT apollo
  DESTINATION share/cmake/apollo)

install(FILES ${PROJECT_BINARY_DIR}/apollo-config.h
  DESTINATION include/apollo)

install(FILES ${PROJECT_BINARY_DIR}/apollo.pc
  DESTINATION lib/pkgconfig)

# ####
#
# External libraries
#
if(ENABLE_MPI)
  find_package(MPI REQUIRED)
  add_definitions(-DENABLE_MPI)
endif()

if(ENABLE_OPENCV)
  find_package(OpenCV REQUIRED)
  include_directories(${OpenCV_INCLUDE_DIRS})
  add_definitions(-DENABLE_OPENCV)
endif()

if(ENABLE_CUDA)
  # TODO: Upgrade to CUDAToolkit for better handling, requires cmake 3.17+
  find_package(CUDA REQUIRED)
  include_directories(${CUDA_INCLUDE_DIRS})
endif()

if(ENABLE_HIP)
  find_package(hip REQUIRED)
  include_directories(${HSA_HEADER})
  add_definitions(-D__HIP_PLATFORM_AMD__)
endif()

if(ENABLE_JIT_DTREE)
  add_definitions(-DENABLE_JIT_DTREE)
endif()

# ####
#
# Define build targets for Apollo
#
include_directories(./src ./include ${PROJECT_BINARY_DIR}/include)

add_subdirectory(src)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif(ENABLE_TESTS)

#
# EOF
