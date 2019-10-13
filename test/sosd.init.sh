#!/bin/bash
if [ "x$SOS_BUILD_DIR" == "x" ] ; then
	echo ""
    echo "ERROR: Please set \$\{SOS_BUILD_DIR\} before running this script."
    echo ""
    kill -INT $$
fi
set +m
rm -f ./sosd.00000.pid
source ${SOS_BUILD_DIR}/../hosts/linux/setenv.sh quiet
export SOS_DB_DISABLED="FALSE"
export SOS_UPDATE_LATEST_FRAME="TRUE"
export SOS_IN_MEMORY_DATABASE="TRUE"
export SOS_EXPORT_DB_AT_EXIT="FALSE"
${SOS_BUILD_DIR}/bin/sosd -l 0 -a 1 -r aggregator -k 0 -w ${SOS_WORK} &
set -m
