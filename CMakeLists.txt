#
# Apollo
#

project(APOLLO LANGUAGES CXX C)
cmake_minimum_required(VERSION 3.1)

#####
#
# Options
#

set(APOLLO_ENABLE_TESTS ON CACHE Bool "")
set(APOLLO_ENABLE_EXAMPLES ON CACHE Bool "")

#
#####

# Version information
set(APOLLO_MAJOR_VERSION 2)
set(APOLLO_MINOR_VERSION 0)
set(APOLLO_PATCH_VERSION 0)
set(APOLLO_VERSION "${APOLLO_MAJOR_VERSION}.${APOLLO_MINOR_VERSION}.${APOLLO_PATCH_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -Wno-unused-variable")

set(APOLLO_BUILD_TYPE "${CMAKE_BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(PreventInSourceBuilds)



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
    message(STATUS "SOS:  Found!")
    message(STATUS "SOS:    SOS_INCLUDE_DIRS = ${SOS_INCLUDE_DIRS}")
    message(STATUS "SOS:    SOS_LIB_DIRS     = ${SOS_LIB_DIRS}")
    message(STATUS "SOS:    SOS_LIBRARY      = ${SOS_LIBRARY}")
    include_directories(${SOS_INCLUDE_DIRS})
    list(APPEND APOLLO_EXTERNAL_LIBS ${SOS_LIBRARY}) 
else()
    message(STATUS "SOS:  Could not locate SOS installation.")
endif(SOS_FOUND)

find_package(Caliper REQUIRED)
if(CALIPER_FOUND)
    message(STATUS "CALIPER:  Found!")
    message(STATUS "CALIPER:    CALIPER_INCLUDE_DIRS = ${CALIPER_INCLUDE_DIRS}")
    message(STATUS "CALIPER:    CALIPER_LIB_DIRS     = ${CALIPER_LIB_DIRS}")
    message(STATUS "CALIPER:    CALIPER_LIBRARY      = ${CALIPER_LIBRARY}")
    include_directories(${CALIPER_INCLUDE_DIRS})
    list(APPEND APOLLO_EXTERNAL_LIBS ${CALIPER_LIBRARY}) 
else()
    message(STATUS "CALIPER:  Could not locate Caliper installation.")
endif(CALIPER_FOUND)

#
#####
#
# Define build targets for Apollo
#

include_directories(./src ${PROJECT_BINARY_DIR}/include)

add_subdirectory(src)

if(APOLLO_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif(APOLLO_ENABLE_EXAMPLES)

if(APOLLO_ENABLE_TESTS)
    add_subdirectory(test)
endif(APOLLO_ENABLE_TESTS)

#
# EOF
