#!/bin/bash
#SBATCH -N 2 
#SBATCH -p pdebug
#SBATCH -A lc 
#SBATCH -t 30

export EXPERIMENT_JOB_TITLE="002.lulesh"
export EXPERIMENT_BASE="/g/g17/wood67/experiments/apollo"
export RETURN_PATH=`pwd`

echo ""
echo "EXPERIMENT_JOB_TITLE: ${EXPERIMENT_JOB_TITLE}"
echo "EXPERIMENT_BASE:      ${EXPERIMENT_BASE}"
echo ""

####
#
#  Launch the SOS runtime:
#
#  Verify the environment has been configured:
source ${RETURN_PATH}/common_unsetenv.sh
source ${RETURN_PATH}/common_setenv.sh
#
export SOS_WORK=${EXPERIMENT_BASE}/${EXPERIMENT_JOB_TITLE}.${SLURM_JOB_ID}
export SOS_EVPATH_MEETUP=${SOS_WORK}/daemons
mkdir -p ${SOS_WORK}
mkdir -p ${SOS_WORK}/launch
mkdir -p ${SOS_WORK}/daemons
#
# Copy the binary, configuration, and plotting scripts into the folder
# where the output of the job is being stored.
#
mkdir -p ${SOS_WORK}/bin
mkdir -p ${SOS_WORK}/lib
#
cp ${HOME}/src/sos_flow/build/bin/sosd                            ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_stop                       ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/src/python/ssos.py                        ${SOS_WORK}/bin
cp ${HOME}/src/apollo/src/python/controller.py                    ${SOS_WORK}/bin
cp ${HOME}/src/apollo/src/python/SQL.CREATE.apolloView            ${SOS_WORK}
#
cp ${HOME}/src/apollo/install/lib/libapollo.so                    ${SOS_WORK}/lib
cp ${HOME}/src/sos_flow/build/lib/libsos.so                       ${SOS_WORK}/lib
cp ${HOME}/src/sos_flow/build/lib/ssos_python.so                  ${SOS_WORK}/lib
cp ${HOME}/src/caliper/install/lib64/libcaliper.so                ${SOS_WORK}/lib
#
export PYTHONPATH=${SOS_WORK}/lib:${SOS_WORK}/bin:${PYTHONPATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SOS_WORK}/lib
#
# Make an archive of this script and the environment config script:
echo "srun ${CONTROLLER_COMMAND}"       > ${SOS_WORK}/launch/CMD_CONTROLLER_COMMAND
echo "srun ${BASELINE_LAUNCH_COMMAND}"  > ${SOS_WORK}/launch/CMD_BASELINE_LAUNCH_COMMAND
echo "srun ${APOLLO_LAUNCH_COMMAND}"    > ${SOS_WORK}/launch/CMD_APOLLO_LAUNCH_COMMAND
echo "srun ${SOS_SHUTDOWN_COMMAND}"     > ${SOS_WORK}/launch/CMD_SOS_SHUTDOWN_COMMAND
cp ${RETURN_PATH}/common_setenv.sh        ${SOS_WORK}/launch/ENV_SOS_SET
cp ${RETURN_PATH}/common_unsetenv.sh      ${SOS_WORK}/launch/ENV_SOS_UNSET
cp ${BASH_SOURCE}                         ${SOS_WORK}/launch/SLURM_BASH_SCRIPT
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

export CONTROLLER_COMMAND="       -N 1 -n 1  -r 0  xterm -e python ./bin/controller.py"
export BASELINE_LAUNCH_COMMAND="  -N 1 -n 32 -r 1  ./bin/lulesh-v2.0-RAJA-seq.exe -q -s 100 -i 10 -b 1 -c 1"
export APOLLO_LAUNCH_COMMAND="    -N 1 -n 32 -r 1  ./bin/lulesh-apollo-base       -q -s 100 -i 10 -b 1 -c 1"
export SOS_SHUTDOWN_COMMAND="     -N 2 -n 2  -r 0  ./bin/sosd_stop"
#
#  Copy the applications into the experiment path:
#
cp ${HOME}/src/raja-proxies/build/bin/lulesh-apollo-base          ${SOS_WORK}/bin
cp ${HOME}/src/raja-proxies/build/bin/lulesh-v2.0-RAJA-seq.exe    ${SOS_WORK}/bin
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
cd ${SOS_WORK}
#
echo ""
echo ">>>> Launching Apollo controller..."
echo ""
srun ${CONTROLLER_COMMAND} &
sleep 1
#
echo ""
echo ">>>> Launching experiment codes..."
echo ""
echo -n "lulesh-raja   "
/usr/bin/time -f %e -- srun ${BASELINE_LAUNCH_COMMAND}
echo -n "lulesh-apollo " 
/usr/bin/time -f %e -- srun ${APOLLO_LAUNCH_COMMAND}
#
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
echo "\$SOS_WORK directory listing:"
tree ${SOS_WORK}
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
cat ${PARTING_NOTE}
echo "srun ${SOS_SHUTDOWN_COMMAND}" > ${SOS_WORK}/sosd_stop.sh
chmod +x ${SOS_WORK}/sosd_stop.sh
#
####
