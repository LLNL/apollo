###############################################################################
#
# Setup SOSflow
#
###############################################################################
#
#  Expects SOS_DIR to point to a SOSflow installation.
#
# This file defines the following CMake variables:
#  SOS_FOUND - If SOSflow was found
#  SOS_INCLUDE_DIRS - The SOSflow include directories
#  SOS_LIBRARY      - Path to the libsos file
#
#  If found, the SOSflow CMake targets will also be imported.
#  The main SOSflow library targets are:
#   sos
#
###############################################################################

###############################################################################
# Check for SOS_DIR
###############################################################################
if(NOT SOS_DIR)
    MESSAGE(FATAL_ERROR "Could not find SOSflow. SOSflow requires explicit SOS_DIR.")
endif()

if(NOT EXISTS ${SOS_DIR}/lib/cmake/sosflow.cmake)
    MESSAGE(FATAL_ERROR "Could not find SOSflow CMake include file (${SOS_DIR}/lib/cmake/sosflow.cmake)")
endif()

###############################################################################
# Import SOSflow's CMake targets
###############################################################################
include(${SOS_DIR}/lib/cmake/sosflow.cmake)

###############################################################################
# Set remaining CMake variables
###############################################################################
# we found SOS
set(SOS_FOUND TRUE)
# provide location of the headers and libraries
set(SOS_INCLUDE_DIRS ${SOS_DIR}/include)
find_library(SOS_LIBRARY NAMES sos HINTS ${SOS_DIR}/lib)
set(SOS_LIB_DIRS ${SOS_DIR}/lib)



