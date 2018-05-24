###############################################################################
#
# Setup Caliper
#
###############################################################################
#
#  Expects CALIPER_DIR to point to a Caliper installation.
#
# This file defines the following CMake variables:
#  CALIPER_FOUND - If Caliper was found
#  CALIPER_INCLUDE_DIRS - The Caliper include directories
#  CALIPER_LIBRARY      - Path to the libcaliper file
#  CALIPER_LIB_DIRS     - Where the library file lives
#
#  If found, the Caliper CMake targets will also be imported.
#  The main Caliper library targets are:
#   caliper
#
###############################################################################

###############################################################################
# Check for CALIPER_DIR
###############################################################################
if(NOT CALIPER_DIR)
    MESSAGE(FATAL_ERROR "Could not find Caliper. Caliper requires explicit CALIPER_DIR.")
endif()

if(NOT EXISTS ${CALIPER_DIR}/share/cmake/caliper/caliper.cmake)
    MESSAGE(FATAL_ERROR "Could not find Caliper CMake include file (${CALIPER_DIR}/share/cmake/caliper/caliper.cmake)")
endif()

###############################################################################
# Import Caliper's CMake targets
###############################################################################
include(${CALIPER_DIR}/share/cmake/caliper/caliper.cmake)

###############################################################################
# Set remaining CMake variables 
###############################################################################
# we found Caliper
set(CALIPER_FOUND TRUE)
# provide location of the headers and libraries
set(CALIPER_INCLUDE_DIRS ${CALIPER_DIR}/include)
find_library(CALIPER_LIBRARY NAMES caliper HINTS ${CALIPER_DIR}/lib)
set(CALIPER_LIB_DIRS ${CALIPER_DIR}/lib)



