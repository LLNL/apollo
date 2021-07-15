# Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
# Apollo Project Developers. See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: (MIT)


###
###  Apollo
###

project(APOLLO LANGUAGES CXX C)
cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 14)


#####
#
# Options
#
option(ENABLE_MPI "Require and enabled MPI for collective training?" ON)
#
set(APOLLO_ENABLE_TESTS    OFF  CACHE STRING "")
set(APOLLO_RPATH_OVERRIDES ON  CACHE STRING "")
set(APOLLO_REQUIRES_PYTHON OFF CACHE STRING "")
#
#####


# Version information
set(APOLLO_MAJOR_VERSION 2)
set(APOLLO_MINOR_VERSION 0)
set(APOLLO_PATCH_VERSION 0)
set(APOLLO_VERSION "${APOLLO_MAJOR_VERSION}.${APOLLO_MINOR_VERSION}.${APOLLO_PATCH_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -Wno-unused-variable -fPIC")

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
set(APOLLO_CXX_COMPILER "${CMAKE_CXX_COMPILER} ")
set(APOLLO_CXX_FLAGS    "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${APOLLO_BUILD_TYPE}} ")
set(APOLLO_LINK_FLAGS   "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_LINK_FLAGS_${CMAKE_BUILD_TYPE}} ")


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

#####
#
# External libraries
#

if (ENABLE_MPI)
    find_package(MPI REQUIRED)
    if (MPI_FOUND)
        message(STATUS "---- MPI:  Found!")
        add_definitions(-DENABLE_MPI)
    else()
        message(STATUS "---- MPI:  ** ERROR ** Could not locate MPI installation.")
    endif()
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

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

#####
#
# Define build targets for Apollo
#

include_directories(./src ./include ${PROJECT_BINARY_DIR}/include)
include_directories("${RAJA_DIR}/include ")

add_subdirectory(src)

if(APOLLO_ENABLE_TESTS)
    add_subdirectory(test)
endif(APOLLO_ENABLE_TESTS)

#
# EOF
