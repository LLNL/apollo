#!/bin/bash
#SBATCH -p pbatch
#SBATCH -A asccasc
#SBATCH --mail-user=wood67@llnl.gov
#SBATCH --mail-type=ALL
#SBATCH --wait=0
#SBATCH --kill-on-bad-exit=0
#SBATCH --requeue
#SBATCH --exclusive
#
#SBATCH --job-name="APOLLO:SCALE.27.lulesh"
#SBATCH -N 2
#SBATCH -n 31
#SBATCH -t 120

export EXPERIMENT_JOB_TITLE="SCALE.0027.lulesh"
export EXPERIMENT_BASE="/p/lustre2/wood67/experiments/apollo"
export RETURN_PATH=`pwd`

echo ""
echo "  JOB TITLE.....: ${EXPERIMENT_JOB_TITLE}"
echo "  WORKING PATH..: ${EXPERIMENT_BASE}/${EXPERIMENT_JOB_TITLE}.${SLURM_JOB_ID}"
echo ""

####
#
#  Launch the SOS runtime:
#
#  Verify the environment has been configured:
source ${RETURN_PATH}/common_unsetenv.sh
#source ${RETURN_PATH}/common_spack.sh
source ${RETURN_PATH}/common_setenv.sh
#
export SOS_WORK=${EXPERIMENT_BASE}/${EXPERIMENT_JOB_TITLE}.${SLURM_JOB_ID}
export SOS_EVPATH_MEETUP=${SOS_WORK}/daemons
mkdir -p ${SOS_WORK}
mkdir -p ${SOS_WORK}/output
mkdir -p ${SOS_WORK}/output/models
mkdir -p ${SOS_WORK}/launch
mkdir -p ${SOS_WORK}/daemons
#
# Copy the binary, configuration, and plotting scripts into the folder
# where the output of the job is being stored.
#
mkdir -p ${SOS_WORK}/bin
mkdir -p ${SOS_WORK}/bin/apollo
mkdir -p ${SOS_WORK}/lib
#
cp ${HOME}/src/sos_flow/build/bin/sosd                            ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_stop                       ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_probe                      ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_manifest                   ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/demo_app                        ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/src/python/ssos.py                        ${SOS_WORK}/bin
#
cp ${HOME}/src/apollo/src/python/controller.py                    ${SOS_WORK}/bin
cp ${HOME}/src/apollo/src/python/apollo/*                         ${SOS_WORK}/bin/apollo
#
cp ${HOME}/src/apollo/jobs/APOLLO.model*                          ${SOS_WORK}
#
cp ${HOME}/src/apollo/src/sql/CREATE.viewApollo                   ${SOS_WORK}
cp ${HOME}/src/apollo/src/sql/CREATE.indexApollo                  ${SOS_WORK}
cp ${HOME}/src/apollo/src/sql/SELECT.sanityCheck                  ${SOS_WORK}
#
cp ${HOME}/src/apollo/install/lib/libapollo.so                    ${SOS_WORK}/lib
cp ${HOME}/src/sos_flow/build/lib/libsos.so                       ${SOS_WORK}/lib
cp ${HOME}/src/sos_flow/build/lib/ssos_python.so                  ${SOS_WORK}/lib
cp ${HOME}/src/caliper/install/lib64/libcaliper.so                ${SOS_WORK}/lib
cp ${HOME}/src/callpath/install/lib/libcallpath.so                ${SOS_WORK}/lib
#
export PYTHONPATH=${SOS_WORK}/lib:${SOS_WORK}/bin:${PYTHONPATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SOS_WORK}/lib
#
# Make an archive of this script and the environment config script:
#
cp ${RETURN_PATH}/common_spack.sh           ${SOS_WORK}/launch/COMMON_SPACK.sh
cp ${RETURN_PATH}/common_setenv.sh          ${SOS_WORK}/launch/COMMON_SETENV.sh
cp ${RETURN_PATH}/common_unsetenv.sh        ${SOS_WORK}/launch/COMMON_UNSETENV.sh
cp ${BASH_SOURCE}                           ${SOS_WORK}/launch/BATCH_JOB_SCRIPT.sh
#
export SOS_BATCH_ENVIRONMENT="TRUE"
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
SOS_DAEMON_TOTAL="2"
#
#
srun -N 1 -n 1 -r 0 ${SOS_WORK}/bin/sosd -k 0 -r aggregator -l 1 -a 1 -w ${SOS_WORK} &
echo "   ... aggregator(0) srun submitted."
for LISTENER_RANK in $(seq 1 1)
do
    srun -N 1 -n 1 -r ${LISTENER_RANK} ${SOS_WORK}/bin/sosd -k ${LISTENER_RANK} -r listener   -l 1 -a 1 -w ${SOS_WORK} &
    echo "   ... listener(${LISTENER_RANK}) srun submitted."
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
#### -------------------------------------------------------------------------
#vvv
#vvv  --- SPECIFIC EXPERIMENT CODE HERE ---
#vv
#v
#
#  Configure Caliper settings for this run:
#
export CALI_LOG_VERBOSITY=0
export CALI_AGGREGATE_KEY="APOLLO_time_flush"
export CALI_SOS_TRIGGER_ATTR="APOLLO_time_flush"
export CALI_SERVICES_ENABLE="sos,timestamp"
export CALI_TIMER_SNAPSHOT_DURATION="false"
export CALI_SOS_ITER_PER_PUBLISH="1"
#
export SRUN_CONTROLLER="       -o ./output/controller.out      -N 1 -n 1  -r 0  python ./bin/controller.py"
export SRUN_LULESH_BASELINE="  -o ./output/lulesh-baseline.out -N 1 -n 27 -r 1  ./bin/lulesh-v2.0-RAJA-seq.exe -p -s 45 -b 1 -c 1"
export SRUN_LULESH_APOLLO="    -o ./output/lulesh-apollo.out   -N 1 -n 27 -r 1  ./bin/lulesh-apollo            -p -s 45 -b 1 -c 1"
export SRUN_SQL_EXEC="         -o /dev/null                    -N 2 -n 2  -r 0  ./bin/demo_app --sql "
export SOS_MONITOR_START="     -o ./daemons/monitor.%n.csv     -N 2 -n 2  -r 0  ./bin/sosd_probe -header on -l 1000000"
export SOS_MONITOR_STOP="                                      -N 2 -n 2  -r 0  killall -9 sosd_probe"
export SOS_SHUTDOWN_COMMAND="                                  -N 2 -n 2  -r 0  ./bin/sosd_stop"
export SQL_DELETE_VALS="DELETE FROM tblVals;"
export SQL_DELETE_DATA="DELETE FROM tblData;"
export SQL_DELETE_PUBS="DELETE FROM tblPubs;"
#
echo "srun ${SRUN_CONTROLLER}"       > ${SOS_WORK}/launch/SRUN_CONTROLLER
echo "srun ${SRUN_LULESH_BASELINE}"  > ${SOS_WORK}/launch/SRUN_LULESH_BASELINE
echo "srun ${SRUN_LULESH_APOLLO}"    > ${SOS_WORK}/launch/SRUN_LULESH_APOLLO
env | grep CALI                      > ${SOS_WORK}/launch/CALIPER_SETTINGS
#
#
#  Copy the applications into the experiment path:
#
cp ${HOME}/src/raja-proxies/build/bin/lulesh-apollo              ${SOS_WORK}/bin
cp ${HOME}/src/raja-proxies/build/bin/lulesh-v2.0-RAJA-seq.exe   ${SOS_WORK}/bin
#
#  Launch an interactive terminal within the allocation:
#
#xterm -fa 'Monospace' -fs 12 -fg grey -bg black &
#
cd ${SOS_WORK}
#
echo ""
echo ">>>> Starting SOS daemon statistics monitoring..."
echo ""
srun ${SOS_MONITOR_START} &
#
echo ""
echo ">>>> Creating Apollo VIEW and INDEX in the SOS databases..."
echo ""
export SQL_APOLLO_VIEW="$(cat CREATE.viewApollo)"
export SQL_APOLLO_INDEX="$(cat CREATE.indexApollo)"
export SQL_APOLLO_SANITY="$(cat SELECT.sanityCheck)"
#srun ${SRUN_SQL_EXEC} SQL_APOLLO_INDEX
srun ${SRUN_SQL_EXEC} SQL_APOLLO_VIEW
#
echo ""
echo ">>>> Default Apollo model..."
echo ""
export APOLLO_INIT_MODEL="${SOS_WORK}/APOLLO.modelDefault"
echo "${APOLLO_INIT_MODEL}"
echo ""

#
#echo ""
#echo ">>>> Launching Apollo controller..."
#echo ""
#srun ${SRUN_CONTROLLER} &
#sleep 1
#
echo ""
echo ">>>> Launching experiment codes..."
echo ""
#
##### --- OpenMP Settings ---
# General:
export KMP_WARNINGS="0"
export KMP_AFFINITY="noverbose,nowarnings,norespect,granularity=fine,explicit"
export KMP_AFFINITY="${KMP_AFFINITY},proclist=[0,1,2,3,4,5,6,7,8,9,10,11"
export KMP_AFFINITY="${KMP_AFFINITY},12,13,14,15,16,17,18,19,20,21,22,23"
export KMP_AFFINITY="${KMP_AFFINITY},24,25,26,27,28,29,30,31,32,33,34,35]"
printf "\nKMP_AFFINITY=${KMP_AFFINITY}\n"
##### --- OpenMP Settings ---

echo ""
printf "\t%4s, %4s, %4s, %-30s, time(sec)\n" "proc" "size" "iter" "application"
#
# NOTE: Only the 2-node script scales PROC like this, the rest are one process
#       count per batch file, since they're requesting more nodes per count.
#
for PROC in $(seq 1 1 3)
do
    for SIZE in $(seq 45 45 45)
    do
        for ITER in $(seq 300 300 1500)
        do
            NUM_RANKS="$((${PROC} ** 3))"
            for LULESH_VARIANT in $(ls ${SOS_WORK}/bin/lulesh*)
            do
                printf "\t%4s, %4s, %4s, %-30s, " ${NUM_RANKS} ${SIZE}  ${ITER} $(basename -- ${LULESH_VARIANT})
                /usr/bin/time -f %e -- srun --cpu-bind=none -c 36 -N 1 -n ${NUM_RANKS} -r 1 ${LULESH_VARIANT} -q -s ${SIZE} -i ${ITER} -b 1 -c 1
                sleep 20
                #srun ${SRUN_SQL_EXEC} ${SQL_DELETE_VALS}
                #srun ${SRUN_SQL_EXEC} ${SQL_DELETE_DATA}
                #srun ${SRUN_SQL_EXEC} ${SQL_DELETE_PUBS}
            done
        done
    done
done
#
#^
#^^
#^^^
#^^^
####
export PARTING_NOTE=${SOS_WORK}/launch/PARTING_INSTRUCTIONS
#
echo "--------------------------------------------------------------------------------"
echo ""
echo "    DONE!"
echo ""
echo "\$SOS_WORK = ${SOS_WORK}"
echo ""
tree ${SOS_WORK}/daemons
ls ${SOS_WORK}
echo "" > ${PARTING_NOTE}
echo "--------------------------------------------------------------------------------"
echo "The SOS RUNTIME IS STILL UP so you can interactively query / run visualizations." >> ${PARTING_NOTE}
echo "" >> ${PARTING_NOTE}
echo "You are now in the \$SOS_WORK directory with your RESULTS and SCRIPTS!" >> ${PARTING_NOTE}
echo "                                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" >> ${PARTING_NOTE}
echo "" >> ${PARTING_NOTE}
echo "        To RETURN to your code ..: $ cd \$RETURN_PATH" >> ${PARTING_NOTE}
echo "        To SHUT DOWN SOS ........: $ ./sosd_stop.sh   (OR: \$ killall srun)" >> ${PARTING_NOTE}
echo "" >> ${PARTING_NOTE}
echo "NOTE: You need to shut down SOS before it will export the databases to files." >> ${PARTING_NOTE}
echo "" >> ${PARTING_NOTE}
cat ${PARTING_NOTE}
echo "echo \"Bringing down SOS:\""      > ${SOS_WORK}/sosd_stop.sh
echo "srun ${SOS_SHUTDOWN_COMMAND}"    >> ${SOS_WORK}/sosd_stop.sh
echo "sleep 2"                         >> ${SOS_WORK}/sosd_stop.sh
echo "echo \"Killing SOS monitors:\""  >> ${SOS_WORK}/sosd_stop.sh
echo "srun ${SOS_MONITOR_STOP}"        >> ${SOS_WORK}/sosd_stop.sh
echo "echo \"OK!\""                    >> ${SOS_WORK}/sosd_stop.sh
chmod +x ${SOS_WORK}/sosd_stop.sh
# So that 'cd -' takes you back to the launch path...
cd ${RETURN_PATH}
cd ${SOS_WORK}
echo ""
echo " >>>>"
echo " >>>>"
echo " >>>> Press ENTER or wait 120 seconds to shut down SOS.   (C-c to stay interactive)"
echo " >>>>"
read -t 120 -p " >>>> "
echo ""
echo " *OK* Shutting down interactive experiment environment..."
echo ""
${SOS_WORK}/sosd_stop.sh
echo ""
echo ""
sleep 20
echo "--- Done! End of job script. ---"
#
# EOF
####
