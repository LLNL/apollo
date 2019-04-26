#!/bin/bash
#
#  Set up caliper to use SOS:
#
export CALI_LOG_VERBOSITY=0
export CALI_AGGREGATE_KEY="APOLLO_time_flush"
export CALI_SOS_TRIGGER_ATTR="APOLLO_time_flush"
export CALI_SERVICES_ENABLE="sos,timestamp"
export CALI_TIMER_SNAPSHOT_DURATION="false"
export CALI_SOS_ITER_PER_PUBLISH="1"
#
echo ""
printf "\t%4s, %4s, %4s, %-30s, time(sec)\n" "proc" "size" "iter" "application"
for SIZE in $(seq 45 45 45)
do
    for ITER in $(seq 300 300 1500)
    do
        for PROC in $(seq 1 1 5)
        do
            NUM_RANKS="$((2 ** ${PROC}))"
            for LULESH_VARIANT in $(ls ${SOS_WORK}/bin/lulesh*)
            do
                printf "\t%4s, %4s, %4s, %-30s, " ${NUM_RANKS} ${SIZE}  ${ITER} $(basename -- ${LULESH_VARIANT}) 
                /usr/bin/time -f %e -- srun -N 1 -n ${NUM_RANKS} -r 1 ${LULESH_VARIANT} -q -s ${SIZE} -i ${ITER} -b 1 -c 1
                sleep 3
            done
        done
    done
done


