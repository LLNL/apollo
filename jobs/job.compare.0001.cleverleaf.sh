#!/bin/bash
#SBATCH -p pbatch
#SBATCH -A asccasc
#SBATCH --mail-user=wood67@llnl.gov
#SBATCH --mail-type=ALL
#SBATCH --requeue
#SBATCH --exclusive
#
#  The following items will need updating at different scales:
#
#SBATCH --job-name="APOLLO:COMPARE.1.cleverleaf.test"
#SBATCH -N 2
#SBATCH -t 300
#
export EXPERIMENT_JOB_TITLE="COMPARE.0001.cleverleaf"  # <-- creates output path!
#
export APPLICATION_RANKS="1"         # ^__ make sure to change SBATCH node counts!
export SOS_AGGREGATOR_COUNT="1"      # <-- actual aggregator count
export EXPERIMENT_NODE_COUNT="2"     # <-- is SBATCH -N count, incl/extra agg. node
#
###################################################################################
#
#  NOTE: Everything below here will get automatically calculated if the above
#        variables are set correctly.
#
#
export  EXPERIMENT_BASE="/p/lustre2/wood67/experiments/apollo"
#
echo ""
echo "  JOB TITLE.....: ${EXPERIMENT_JOB_TITLE}"
echo "  WORKING PATH..: ${EXPERIMENT_BASE}/${EXPERIMENT_JOB_TITLE}.${SLURM_JOB_ID}"
echo ""
#
####


export RETURN_PATH=`pwd`

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
mkdir -p ${SOS_WORK}/lib
mkdir -p ${SOS_WORK}/bin
mkdir -p ${SOS_WORK}/bin/apollo
#
cp ${HOME}/src/sos_flow/build/bin/sosd                            ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_stop                       ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_probe                      ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_manifest                   ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/demo_app                        ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/scripts/showdb                            ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/src/python/ssos.py                        ${SOS_WORK}/bin
#
cp ${HOME}/src/apollo/src/python/controller.py                    ${SOS_WORK}/bin
cp ${HOME}/src/apollo/src/python/apollo/*                         ${SOS_WORK}/bin/apollo
#
cp ${HOME}/src/apollo/jobs/model.*                                ${SOS_WORK}
#
cp ${HOME}/src/apollo/src/python/SQL.CREATE.viewApollo            ${SOS_WORK}
cp ${HOME}/src/apollo/src/python/SQL.CREATE.indexApollo           ${SOS_WORK}
cp ${HOME}/src/apollo/src/python/SQL.sanityCheck                  ${SOS_WORK}
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
env | grep CALI > ${SOS_WORK}/launch/CALIPER_SETTINGS
#
#  Set up the commands we'll use for this experiment kit:
#
let    WORK_NODE_COUNT=$[$EXPERIMENT_NODE_COUNT - 1]
#
export SRUN_CONTROLLER_START=" "
export SRUN_CONTROLLER_START+=" -o ./output/controller.out "
export SRUN_CONTROLLER_START+=" --open-mode=append "
export SRUN_CONTROLLER_START+=" -N 1 -n 1 -r 0 "
export SRUN_CONTROLLER_START+=" python ./bin/controller.py "
#
export SRUN_CONTROLLER_STOP=" "
export SRUN_CONTROLLER_STOP+=" -o /dev/null "
export SRUN_CONTROLLER_STOP+=" -N 1 -n 1 -r 0 "
export SRUN_CONTROLLER_STOP+=" killall -9 python "
#
export SRUN_SQL_EXEC=" "
export SRUN_SQL_EXEC+=" -o ./output/sqlexec.%n.out "
export SRUN_SQL_EXEC+=" --open-mode=append "
export SRUN_SQL_EXEC+=" -N ${EXPERIMENT_NODE_COUNT} "
export SRUN_SQL_EXEC+=" -n ${EXPERIMENT_NODE_COUNT} "
export SRUN_SQL_EXEC+=" -r 0 "
export SRUN_SQL_EXEC+=" ./bin/demo_app --sql SOS_SQL "
#
export SOS_MONITOR_START=" "
export SOS_MONITOR_START+=" -o ./daemons/monitor.%n.csv "
export SOS_MONITOR_START+=" -N ${EXPERIMENT_NODE_COUNT} "
export SOS_MONITOR_START+=" -n ${EXPERIMENT_NODE_COUNT} "
export SOS_MONITOR_START+=" -r 0 "
export SOS_MONITOR_START+=" ./bin/sosd_probe -header on -l 1000000 "
#
export SOS_MONITOR_STOP=" "
export SOS_MONITOR_STOP+=" -N ${EXPERIMENT_NODE_COUNT} "
export SOS_MONITOR_STOP+=" -n ${EXPERIMENT_NODE_COUNT} "
export SOS_MONITOR_STOP+=" -r 0 "
export SOS_MONITOR_STOP+=" killall -9 sosd_probe "
#
export SOS_SHUTDOWN_COMMAND=" "
export SOS_SHUTDOWN_COMMAND+=" -N ${EXPERIMENT_NODE_COUNT} "
export SOS_SHUTDOWN_COMMAND+=" -n ${EXPERIMENT_NODE_COUNT} "
export SOS_SHUTDOWN_COMMAND+=" -r 0 "
export SOS_SHUTDOWN_COMMAND+=" ./bin/sosd_stop "
#
export SQL_DELETE_VALS="DELETE FROM tblVals;"
export SQL_DELETE_DATA="DELETE FROM tblData;"
export SQL_DELETE_PUBS="DELETE FROM tblPubs;"
#
#
echo "srun ${SRUN_CONTROLLER}"       > ${SOS_WORK}/launch/SRUN_CONTROLLER
env | grep SLURM                     > ${SOS_WORK}/launch/SLURM_ENV
#
#  Copy the applications into the experiment path:
#
cp ${HOME}/src/cleverleaf/package-apollo/RelWithDebInfo/install/cleverleaf/bin/cleverleaf \
    ${SOS_WORK}/bin/cleverleaf-apollo
cp ${HOME}/src/cleverleaf/package-normal/RelWithDebInfo/install/cleverleaf/bin/cleverleaf \
    ${SOS_WORK}/bin/cleverleaf-normal

#
#  Bring over the input deck[s]:
cp ${HOME}/src/apollo/jobs/cleaf*.in   ${SOS_WORK}
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
export SQL_APOLLO_VIEW=`cat SQL.CREATE.viewApollo`
export SQL_APOLLO_INDEX=`cat SQL.CREATE.indexApollo`
#
SOS_SQL=${SQL_APOLLO_VIEW} srun ${SRUN_SQL_EXEC}
#SOS_SQL=${SQL_APOLLO_INDEX} srun ${SRUN_SQL_EXEC}
#
export SQL_SANITYCHECK=`cat SQL.sanityCheck`
#
echo ""
echo ">>>> Launching experiment codes..."
echo ""
#
    export CLEVERLEAF_APOLLO_BINARY=" ${SOS_WORK}/bin/cleverleaf-apollo "
    export CLEVERLEAF_NORMAL_BINARY=" ${SOS_WORK}/bin/cleverleaf-normal "

    export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_25.in"
    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_50.in"
    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_100.in"
    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_500.in"
    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_test.in"

    export SRUN_CLEVERLEAF=" "
    export SRUN_CLEVERLEAF+=" --cpu-bind=cores "
    export SRUN_CLEVERLEAF+=" -c 36 "
    #export SRUN_CLEVERLEAF+=" -o ${SOS_WORK}/output/cleverleaf.%4t.stdout "
    export SRUN_CLEVERLEAF+=" -o /dev/null "
    export SRUN_CLEVERLEAF+=" -N ${WORK_NODE_COUNT} "
    export SRUN_CLEVERLEAF+=" -n ${APPLICATION_RANKS} "
    export SRUN_CLEVERLEAF+=" -r 1 "

    echo ">>>> Comparing cleverleaf-normal and cleverleaf-apollo..."

    echo ""
    echo "========== EXPERIMENTS STARTING =========="
    echo ""

    function wipe_all_sos_data_from_database() {
        echo "========== BEGIN $(basename -- ${APOLLO_INIT_MODEL}) ==========" \
            >> ./output/sqlexec.out
        SOS_SQL=${SQL_DELETE_VALS} srun ${SRUN_SQL_EXEC}
        SOS_SQL=${SQL_DELETE_DATA} srun ${SRUN_SQL_EXEC}
        SOS_SQL=${SQL_DELETE_PUBS} srun ${SRUN_SQL_EXEC}
        SOS_SQL="VACUUM;" srun ${SRUN_SQL_EXEC}
    }

    function run_cleverleaf_with_model() {
        export APOLLO_INIT_MODEL="${SOS_WORK}/$3"
        #echo "========== BEGIN $(basename -- ${APOLLO_INIT_MODEL}) ==========" \
        #    >> ./output/cleverleaf.0000.stdout
        #wipe_all_sos_data_from_database
        cd output

        #echo ""
        #echo "srun ${SRUN_CLEVERLEAF} $1 $2"
        #echo ""

        printf "\t%4s, %-20s, %-30s, " ${APPLICATION_RANKS} \
            $(basename -- ${CLEVERLEAF_INPUT}) $(basename -- ${APOLLO_INIT_MODEL})
        /usr/bin/time -f %e -- srun ${SRUN_CLEVERLEAF} $1 $2
        cd ${SOS_WORK}
    }

set +m

    run_cleverleaf_with_model ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} "model.previous"
    run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.........default"

    # The static model doesn't adjust anything, and doesn't receive feedback from the controller,
    # there is no need to start it until now. SOS is still running and receiving data.
    #srun ${SRUN_CONTROLLER_START} &
    #sleep 4

#    export OMP_DISPLAY_ENV=VERBOSE
#    run_cleverleaf_with_model ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} "model.previous"

#    export OMP_NUM_THREADS=32
#    export OMP_SCHEDULE="auto"
#    run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"
#
#    export OMP_NUM_THREADS=16
#    export OMP_SCHEDULE="auto"
#    run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"
#
#    export OMP_NUM_THREADS=8
#    export OMP_SCHEDULE="auto"
#    run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"
#
#    export OMP_NUM_THREADS=4
#    export OMP_SCHEDULE="auto"
#    run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"
#
#    export OMP_NUM_THREADS=2
#    export OMP_SCHEDULE="auto"
#    run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"

    cd ${SOS_WORK}

    echo ""
    echo "========== EXPERIMENTS COMPLETE =========="
    echo ""

    #echo ""
    #echo ">>>> Bringing down the controller and waiting for 5 seconds (you may see 'kill' output)..."
    #echo ""
    #printf "== CONTROLLER: STOP\n" >> ./output/controller.out
    #srun ${SRUN_CONTROLLER_STOP}
    #echo ""
    #sleep 5

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
ls ${SOS_WORK}/output
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
echo "Constructing / emailing a results summary:"
${RETURN_PATH}/end.emailresults.sh cleverleaf
echo ""
echo ""
set -m
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
