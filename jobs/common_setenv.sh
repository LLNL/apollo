#!/bin/bash

# Absolute path this script is in:
export SETENV_SCRIPT_PATH="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"

if [ "x${SOS_DIR}" == "x" ] ; then
    export SOS_DIR=${HOME}/src/sos_flow
fi

#if [ "x$SOS_ENV_SET" == "x1" ] ; then
#	echo "WARNING: You SOS environment is already configured."
#    echo "         Please run the unsetenv.sh script first:"
#    echo ""
#    echo "    $SETENV_SCRIPT_PATH/unsetenv.sh"
#    echo ""
#    kill -INT $$
#fi
#
# Runtime options for SOS daemons:
#
export SOS_CMD_PORT=22500
export SOS_BUILD_DIR=$SOS_DIR/build
export SOS_OPTIONS_FILE=""
export SOS_SHUTDOWN="FALSE"
export SOS_FWD_SHUTDOWN_TO_AGG="TRUE"
export SOS_UDP_ENABLED="FALSE"
export SOS_DB_DISABLED="FALSE"
export SOS_UPDATE_LATEST_FRAME="TRUE"
export SOS_IN_MEMORY_DATABASE="TRUE"
export SOS_EXPORT_DB_AT_EXIT="VERBOSE"
export SOS_PUB_CACHE_DEPTH="0"
export SOS_SYSTEM_MONITOR_ENABLED="FALSE"
export SOS_SYSTEM_MONITOR_FREQ_USEC="0"
export SOS_ENV_SET=1
#
#
#
#
