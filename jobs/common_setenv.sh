#!/bin/bash

# Absolute path this script is in:
export SETENV_SCRIPT_PATH="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"

if [ "x$SOS_ENV_SET" == "x1" ] ; then
	echo "WARNING: You SOS environment is already configured."
    echo "         Please run the unsetenv.sh script first:"
    echo ""
    echo "    $SETENV_SCRIPT_PATH/unsetenv.sh"
    echo ""
    kill -INT $$
fi

# Runtime options
export SOS_CMD_PORT=22500
export SOS_BASE=$HOME/src/sos_flow
export SOS_BUILD_DIR=$SOS_BASE/build
export SOS_WORK="/g/g17/wood67/experiments/apollo"
export SOS_EVPATH_MEETUP=$SOS_WORK
export SOS_OPTIONS_FILE=""
export SOS_SHUTDOWN="FALSE"
export SOS_FWD_SHUTDOWN_TO_AGG="TRUE"
export SOS_UDP_ENABLED="FALSE"
export SOS_DB_DISABLED="FALSE"
export SOS_UPDATE_LATEST_FRAME="TRUE"
export SOS_IN_MEMORY_DATABASE="FALSE"
export SOS_EXPORT_DB_AT_EXIT="FALSE"
export SOS_PUB_CACHE_DEPTH="0"
export SOS_SYSTEM_MONITOR_ENABLED="FALSE"
export SOS_SYSTEM_MONITOR_FREQ_USEC="0"
export SOS_ENV_SET=1

#
#  NOTE: If we decide to set these paths for experiments...
#        ...it makes unsetting the environment a bit trickier.
#
#echo "Updating paths:"
#echo "     \$PATH            += \$SOS_BASE/scripts"
#export PATH="${PATH}:$SOS_BASE/scripts"
#
#echo "     \$PYTHONPATH      += \$SOS_BUILD_DIR/lib"
#if [ "x$PYTHONPATH" == "x" ] ; then
#  PYTHONPATH=$SOS_BASE/src/python
#else
#  PYTHONPATH=$SOS_BASE/src/python:$PYTHONPATH
#fi
#
#echo "     \$LD_LIBRARY_PATH += \$SOS_BUILD_DIR/lib"
#if [ "x$LD_LIBRARY_PATH" == "x" ] ; then
#  LD_LIBRARY_PATH=$SOS_BUILD_DIR/lib
#else
#  LD_LIBRARY_PATH=$SOS_BUILD_DIR/lib:$LD_LIBRARY_PATH
#fi
#

