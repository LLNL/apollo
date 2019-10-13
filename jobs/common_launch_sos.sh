#!/bin/bash
#
export SOS_BATCH_ENVIRONMENT="TRUE"
export SOS_EVPATH_MEETUP=${SOS_WORK}/daemons
#
if [ "x${SOS_ENV_SET}" == "x" ] ; then
    echo "Please set up your SOS environment first."
    kill -INT $$
fi
if ls ${SOS_EVPATH_MEETUP}/sosd.*.key 1> /dev/null 2>&1
then
    echo "WARNING: Aggregator KEY and ID file[s] exist already.  Deleting them."
    rm -f ${SOS_EVPATH_MEETUP}/sosd.*.key
    rm -f ${SOS_EVPATH_MEETUP}/sosd.*.id
fi
if ls ${SOS_WORK}/sosd.*.db 1> /dev/null 2>&1
then
    echo "WARNING: SOSflow DATABASE file[s] exist already.  Deleting them."
    rm -rf ${SOS_WORK}/sosd.*.db
    rm -rf ${SOS_WORK}/sosd.*.db.export
    rm -rf ${SOS_WORK}/sosd.*.db.lock
    rm -rf ${SOS_WORK}/sosd.*.db-journal
fi
#
echo ""
echo "Launching SOS daemons..."
echo ""
echo "    SOS_WORK=${SOS_WORK}"
echo "    SOS_EVPATH_MEETUP=${SOS_EVPATH_MEETUP}"
echo ""
#
#
let SOS_LISTENER_COUNT=$[$EXPERIMENT_NODE_COUNT - 1]
let SOS_DAEMON_TOTAL=$[$SOS_AGGREGATOR_COUNT + $SOS_LISTENER_COUNT]
#
#
for AGGREGATOR_RANK in $(seq 0 $[$SOS_AGGREGATOR_COUNT - 1])
do
    srun -N 1 -n 1 -r 0 \
        ${SOS_WORK}/bin/sosd \
        -k ${AGGREGATOR_RANK} \
        -r aggregator \
        -l ${SOS_LISTENER_COUNT} \
        -a ${SOS_AGGREGATOR_COUNT} \
        -w ${SOS_WORK} &
    echo "  --> sosd.aggregator(${AGGREGATOR_RANK}) starting..."
done
for WORK_NODE_INDEX in $(seq 1 $[$EXPERIMENT_NODE_COUNT - 1])
do
    let LISTENER_RANK=$[$SOS_AGGREGATOR_COUNT - 1 + $WORK_NODE_INDEX]
    srun -N 1 -n 1 -r ${WORK_NODE_INDEX} \
        ${SOS_WORK}/bin/sosd \
        -k ${LISTENER_RANK} \
        -r listener \
        -l ${SOS_LISTENER_COUNT} \
        -a ${SOS_AGGREGATOR_COUNT} \
        -w ${SOS_WORK} &
    echo "  --> sosd.listener(${LISTENER_RANK}) starting..."
done
#
#
echo ""
echo "Pausing to ensure runtime is completely established..."
echo ""
SOS_DAEMONS_SPAWNED="0"
while [ ${SOS_DAEMONS_SPAWNED} -lt ${SOS_DAEMON_TOTAL} ]
do

    if ls ${SOS_EVPATH_MEETUP}/sosd.*.id 1> /dev/null 2>&1
    then
        SOS_DAEMONS_SPAWNED="$(ls -l ${SOS_EVPATH_MEETUP}/sosd.*.id | grep -v ^d | wc -l)"
    else
        SOS_DAEMONS_SPAWNED="0"
    fi

    if [ "x${SOS_BATCH_ENVIRONMENT}" == "x" ]; then
        for STEP in $(seq 1 20)
        do
            echo -n "  ["
            for DOTS in $(seq 1 ${STEP})
            do
                echo -n "#"
            done
            for SPACES in $(seq ${STEP} 19)
            do
                echo -n " "
            done
            echo -n "]  ${SOS_DAEMONS_SPAWNED} of ${SOS_DAEMON_TOTAL} daemons running..."
            echo -ne "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
            echo -ne "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
            sleep 0.1
        done
    fi
done
echo ""
echo ""
echo "SOS is ready for use!"
echo ""
echo ""
echo "--------------------------------------------------------------------------------"
echo ""
#
