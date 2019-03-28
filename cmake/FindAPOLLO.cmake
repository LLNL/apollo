###############################################################################
#
# Setup APOLLO
#
###############################################################################
#
#  Expects APOLLO_DIR to point to an Apollo installation.
#
# This file defines the following CMake variables:
#  APOLLO_FOUND        - If Apollo was found
#  APOLLO_INCLUDE_DIRS - The Apollo include directories
#  APOLLO_LIBRARY      - Path to the libapollo file
#
#  If found, the Apollo CMake targets will also be imported.
#  The main Apollo library targets are:
#   apollo
#
###############################################################################

###############################################################################
# Check for APOLLO_DIR
###############################################################################
if(NOT APOLLO_DIR)
    MESSAGE(FATAL_ERROR "Could not find Apollo. Apollo requires explicit APOLLO_DIR.")
endif()

if(NOT EXISTS ${APOLLO_DIR}/lib/cmake/apollo.cmake)
    MESSAGE(FATAL_ERROR "Could not find Apollo CMake include file (${APOLLO_DIR}/lib/cmake/apollo.cmake)")
endif()

###############################################################################
# Import APOLLO's CMake targets
###############################################################################
include(${APOLLO_DIR}/lib/cmake/apollo.cmake)

###############################################################################
# Set remaining CMake variables 
###############################################################################
# We found Apollo
set(APOLLO_FOUND TRUE)
# Provide location of the headers and libraries
set(APOLLO_INCLUDE_DIRS ${APOLLO_DIR}/include)
find_library(APOLLO_LIBRARY NAMES apollo HINTS ${APOLLO_DIR}/lib)
set(APOLLO_LIB_DIRS ${APOLLO_DIR}/lib)



