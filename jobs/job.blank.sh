#!/bin/bash
#
export EXPERIMENT_JOB_TITLE="BLANK.test.cleverleaf"  # <-- creates output path!
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
export  EXPERIMENT_BASE="/p/lustre2/${USER}/experiments/apollo"
#
export  SOS_WORK=${EXPERIMENT_BASE}/${EXPERIMENT_JOB_TITLE}.${SLURM_JOB_ID}
export  SOS_EVPATH_MEETUP=${SOS_WORK}/daemons
#
echo ""
echo "  JOB TITLE.....: ${EXPERIMENT_JOB_TITLE}"
echo "  WORKING PATH..: ${SOS_WORK}"
echo ""
#
export RETURN_PATH=`pwd`

####
#
#
source ${RETURN_PATH}/common_unsetenv.sh
source ${RETURN_PATH}/common_spack.sh
source ${RETURN_PATH}/common_setenv.sh
source ${RETURN_PATH}/common_copy_files.sh
source ${RETURN_PATH}/common_launch_sos.sh
#
#
#### -------------------------------------------------------------------------
#
#  Configure Caliper settings for this run:
#
#export CALI_LOG_VERBOSITY=0
#export CALI_AGGREGATE_KEY="APOLLO_time_flush"
#export CALI_SOS_TRIGGER_ATTR="APOLLO_time_flush"
#export CALI_SERVICES_ENABLE="sos,timestamp"
#export CALI_TIMER_SNAPSHOT_DURATION="false"
#export CALI_SOS_ITER_PER_PUBLISH="1"
#
#env | grep -F "CALI_" > ${SOS_WORK}/launch/caliper_env_settings
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
    export CLEVERLEAF_APOLLO_BINARY=" ${SOS_WORK}/bin/cleverleaf-apollo-release "
    export CLEVERLEAF_NORMAL_BINARY=" ${SOS_WORK}/bin/cleverleaf-normal-release "

    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_50.in"
    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_100.in"
    export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_500.in"
    #export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_test.in"

    export SRUN_CLEVERLEAF=" "
    export SRUN_CLEVERLEAF+=" --cpu-bind=cores "
    export SRUN_CLEVERLEAF+=" -c 32 "
    export SRUN_CLEVERLEAF+=" -o ${SOS_WORK}/output/cleverleaf.%4t.stdout "
    export SRUN_CLEVERLEAF+=" -N ${WORK_NODE_COUNT} "
    export SRUN_CLEVERLEAF+=" -n ${APPLICATION_RANKS} "
    export SRUN_CLEVERLEAF+=" -r 1 "


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
        echo "========== BEGIN $(basename -- ${APOLLO_INIT_MODEL}) ==========" \
            >> ./output/cleverleaf.0000.stdout
        wipe_all_sos_data_from_database
        cd output

        echo ""
        echo "srun ${SRUN_CLEVERLEAF} $1 $2"
        echo ""

        printf "\t%4s, %-20s, %-30s, " ${APPLICATION_RANKS} \
            $(basename -- ${CLEVERLEAF_INPUT}) $(basename -- ${APOLLO_INIT_MODEL})
        /usr/bin/time -f %e -- srun ${SRUN_CLEVERLEAF} $1 $2
        cd ${SOS_WORK}
        echo "SANITY CHECK:" \
            >> ./output/sqlexec.out
        SOS_SQL=${SQL_SANITYCHECK} srun ${SRUN_SQL_EXEC}
    }

    #run_cleverleaf_with_model ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} "model.static.0.default"
    #run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.........default"


    # The static model doesn't adjust anything, and doesn't receive feedback from the controller,
    # there is no need to start it until now. SOS is still running and receiving data.
    #srun ${SRUN_CONTROLLER_START} &
    #sleep 4

    #export OMP_DISPLAY_ENV=VERBOSE
    #run_cleverleaf_with_model ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} "model.roundrobin"

    #export OMP_NUM_THREADS=32
    #export OMP_SCHEDULE="auto"
    #run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"

    #export OMP_NUM_THREADS=16
    #export OMP_SCHEDULE="auto"
    #run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"

    #export OMP_NUM_THREADS=8
    #export OMP_SCHEDULE="auto"
    #run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"

    #export OMP_NUM_THREADS=4
    #export OMP_SCHEDULE="auto"
    #run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"

    #export OMP_NUM_THREADS=2
    #export OMP_SCHEDULE="auto"
    #run_cleverleaf_with_model ${CLEVERLEAF_NORMAL_BINARY} ${CLEVERLEAF_INPUT} "normal.${OMP_NUM_THREADS}.${OMP_SCHEDULE}"

    export OMP_NUM_THREADS=36
    export APOLLO_INIT_MODEL=${SOS_WORK}/model.static.0.default
    export OMP_DISPLAY_ENV=verbose
    cd ${SOS_WORK}
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
#echo "Constructing / emailing a results summary:"
#${RETURN_PATH}/end.emailresults.sh cleverleaf
echo ""
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
