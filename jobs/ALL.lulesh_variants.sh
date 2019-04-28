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
export SQL_DELETE_VALS="DELETE FROM tblVals;"
export SQL_DELETE_DATA="DELETE FROM tblData;"
export SQL_DELETE_PUBS="DELETE FROM tblPubs;"
export SRUN_SQL_EXEC="         -o /dev/null                    -N 2 -n 2  -r 0  ./bin/demo_app --sql "
#
echo ""
printf "\t%4s, %4s, %4s, %-30s, time(sec)\n" "proc" "size" "iter" "application"
for SIZE in $(seq 45 45 45)
do
    for ITER in $(seq 300 300 1500)
    do
        for PROC in $(seq 1 1 3)
        do
            NUM_RANKS="$((${PROC} ** 3))"
            for LULESH_VARIANT in $(ls ${SOS_WORK}/bin/lulesh*)
            do
                printf "\t%4s, %4s, %4s, %-30s, " ${NUM_RANKS} ${SIZE}  ${ITER} $(basename -- ${LULESH_VARIANT}) 
                /usr/bin/time -f %e -- srun -N 1 -n ${NUM_RANKS} -r 1 ${LULESH_VARIANT} -q -s ${SIZE} -i ${ITER} -b 1 -c 1
                sleep 10 
                srun ${SRUN_SQL_EXEC} ${SQL_DELETE_VALS}
                srun ${SRUN_SQL_EXEC} ${SQL_DELETE_DATA}
                srun ${SRUN_SQL_EXEC} ${SQL_DELETE_PUBS}
            done
        done
    done
done


