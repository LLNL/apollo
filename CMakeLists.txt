cmake_minimum_required(VERSION 3.1)

project(APOLLO LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -Wno-unused-variable")

set(APOLLO_BUILD_TYPE "${CMAKE_BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(PreventInSourceBuilds)

set(BLT_CXX_STD c++14)

#
# Initialize the BLT build system
#

set(ENABLE_GTEST Off CACHE Bool "")
set(ENABLE_TESTS Off CACHE Bool "")
set(ENABLE_TBB Off CACHE Bool "")
set(ENABLE_EXAMPLES Off CACHE Bool "")
set(ENABLE_DOCUMENTATION Off CACHE Bool "")

include(blt/SetupBLT.cmake)


#
# Configure APOLLO
#

set(APOLLO_ENABLE_TESTS Off CACHE Bool "")

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

configure_file(${PROJECT_SOURCE_DIR}/src/ApolloConfig.h.in
               ${PROJECT_BINARY_DIR}/include/ApolloConfig.h)


find_package(SOS REQUIRED)
if(SOS_FOUND)
    message(STATUS "SOS:  Found!")
    message(STATUS "SOS:    SOS_INCLUDE_DIRS = ${SOS_INCLUDE_DIRS}")
    message(STATUS "SOS:    SOS_LIB_DIRS     = ${SOS_LIB_DIRS}")
    message(STATUS "SOS:    SOS_LIBRARY      = ${SOS_LIBRARY}")
else()
    message(STATUS "SOS:  Could not locate SOS installation.")
endif(SOS_FOUND)

#
# Define build targets for Apollo
#
include_directories(./src ${PROJECT_BINARY_DIR}/include)

blt_add_library(
  NAME        apollo 
  SOURCES     "src/Apollo.cpp"
  DEPENDS_ON  ${APOLLO_DEPENDS}
              ${SOS_LIBRARY}
)
if (SOS_FOUND)
    include_directories(${SOS_INCLUDE_DIRS})
endif(SOS_FOUND)
get_property(APOLLO_INCLUDE_DIRS TARGET apollo PROPERTY INCLUDE_DIRECTORIES)
include_directories(${APOLLO_INCLUDE_DIRS})


