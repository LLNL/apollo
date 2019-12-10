
# Copyright (c) 2019, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory
#
# This file is part of Apollo.
# OCEC-17-092
# All rights reserved.
#
# Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
# of many collaborators.
#
# Apollo was originally created by David Beckingsale, david@llnl.gov
#
# For details, see https://github.com/LLNL/apollo.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

#
# Apollo
#

project(APOLLO LANGUAGES CXX C)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)


#####
#
# Options
#

set(APOLLO_ENABLE_TESTS    ON  CACHE STRING "")
set(APOLLO_ENABLE_EXAMPLES OFF CACHE STRING "")
set(APOLLO_RPATH_OVERRIDES ON  CACHE STRING "")
set(APOLLO_REQUIRES_PYTHON OFF CACHE STRING "")
#
#####

# Version information
set(APOLLO_MAJOR_VERSION 2)
set(APOLLO_MINOR_VERSION 0)
set(APOLLO_PATCH_VERSION 0)
set(APOLLO_VERSION "${APOLLO_MAJOR_VERSION}.${APOLLO_MINOR_VERSION}.${APOLLO_PATCH_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -fPIC -qopt-report=5 -qopt-report-file=${OPT_REPORT_FILE} ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -Wno-unused-variable -fPIC  ")

set(APOLLO_BUILD_TYPE "${CMAKE_BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(PreventInSourceBuilds)

if (APOLLO_RPATH_OVERRIDES)
	# use, i.e. don't skip the full RPATH for the build tree
	set(CMAKE_SKIP_BUILD_RPATH  FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


	# the RPATH to be used when installing, but only if it's not a system directory
	list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
	if("${isSystemDir}" STREQUAL "-1")
   		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	endif("${isSystemDir}" STREQUAL "-1")
endif(APOLLO_RPATH_OVERRIDES)



#
# Setup variables to pass to ApolloConfig.h
#
# Pull in macros
include(ApolloProjectMacros)
# Pull in the GIT repo information
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
#add_definitions("-DAPOLLO_GIT_SHA1=${GIT_SHA1}")
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

#string(TOUPPER ${APOLLO_BUILD_TYPE} CMAKE_BUILD_TYPE)
set(APOLLO_CXX_COMPILER "${CMAKE_CXX_COMPILER}")
set(APOLLO_CXX_FLAGS    "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${APOLLO_BUILD_TYPE}}")
set(APOLLO_LINK_FLAGS   "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_LINK_FLAGS_${CMAKE_BUILD_TYPE}}")


# Create a config header file
configure_file(
    ${PROJECT_SOURCE_DIR}/apollo-config.h.in
    ${PROJECT_BINARY_DIR}/apollo-config.h)

# Create pkg-confic .pc file
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_LIBS "-L\${libdir} -lapollo")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

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

#####
#
# External libraries
#

find_package(SOS REQUIRED)
if(SOS_FOUND)
    message(STATUS "---- SOS:  Found!")
    message(STATUS "---- SOS:    SOS_INCLUDE_DIRS = ${SOS_INCLUDE_DIRS}")
    message(STATUS "---- SOS:    SOS_LIB_DIRS     = ${SOS_LIB_DIRS}")
    message(STATUS "---- SOS:    SOS_LIBRARY      = ${SOS_LIBRARY}")
    include_directories(${SOS_INCLUDE_DIRS})
    list(APPEND APOLLO_EXTERNAL_LIBS ${SOS_LIBRARY})
else()
    message(STATUS "---- SOS:  ** ERROR ** Could not locate SOS installation.")
endif(SOS_FOUND)

find_package(MPI REQUIRED)
if (MPI_FOUND)
    message(STATUS "---- MPI:  Found!")
endif()

find_package(OpenMP)
if (OPENMP_FOUND)
    message(STATUS "---- OPENMP:  Found!")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} ")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    message(STATUS "---- OPENMP:  Not found.")
endif(OPENMP_FOUND)


find_package(callpath REQUIRED)
if(callpath_CONFIG_LOADED)
    message(STATUS "---- CALLPATH:  Found!")
    message(STATUS "---- CALLPATH:    callpath_INCLUDE_DIR = ${callpath_INCLUDE_DIR}")
    message(STATUS "---- CALLPATH:    callpath_LIB_DIR     = ${callpath_LIB_DIR}")
    include_directories(${callpath_INCLUDE_DIR})
    list(APPEND APOLLO_EXTERNAL_LIBS ${callpath_LIB_DIR}/libcallpath.so)
else()
    message(STATUS "---- CALLPATH:  ** ERROR ** Could not locate the callpath library.")
endif(callpath_CONFIG_LOADED)


if(APOLLO_REQUIRES_PYTHON)
    find_package(PythonInterp 2.7 REQUIRED)
    find_packagE(PythonLibs 2.7 REQUIRED)
    if(PYTHONLIBS_FOUND)
        message(STATUS "---- PYTHON:  Found!")
        message(STATUS "---- PYTHON:    PYTHON_INCLUDE_DIRS       = ${PYTHON_INCLUDE_DIRS}")
        message(STATUS "---- PYTHON:    PYTHON_LIBRARIES          = ${PYTHON_LIBRARIES}")
        message(STATUs "---- PYTHON:    PYTHONLIBS_VERSION_STRING = ${PYTHONLIBS_VERSION_STRING}")
        include_directories(${PYTHON_INCLUDE_DIRS})
        list(APPEND APOLLO_EXTERNAL_LIBS ${PYTHON_LIBRARIES})
    else()
        message(STATUS "---- PYTHON:  ** ERROR ** Could not locate Python installation.")
    endif(PYTHONLIBS_FOUND)

endif(APOLLO_REQUIRES_PYTHON)

# JSON parser (nlohmann-json)
include_directories(./include/nlohmann)
message(STATUS "---- JSON:  Found!")

#####
#
# Define build targets for Apollo
#

include_directories(./src ./include ${PROJECT_BINARY_DIR}/include)
include_directories("${RAJA_DIR}/include")

add_subdirectory(src)

if(APOLLO_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif(APOLLO_ENABLE_EXAMPLES)

if(APOLLO_ENABLE_TESTS)
    add_subdirectory(test)
endif(APOLLO_ENABLE_TESTS)

#
# EOF
