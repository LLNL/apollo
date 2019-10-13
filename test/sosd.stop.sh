#!/bin/bash
if [ "x$SOS_BUILD_DIR" == "x" ] ; then
	echo ""
    echo "ERROR: Please set \$\{SOS_BUILD_DIR\} before running this script."
    echo ""
    kill -INT $$
fi
set +m
${SOS_BUILD_DIR}/bin/sosd_stop
rm -f ./sosd.00000.id
set -m
