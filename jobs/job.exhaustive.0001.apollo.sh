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
#SBATCH --job-name="APOLLO:EXHAUSTIVE.0001.apollo"
#SBATCH -N 2
#SBATCH -t 120
#
export EXPERIMENT_JOB_TITLE="EXHAUSTIVE.0001.apollo"  # <-- creates output path!
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
#source ${RETURN_PATH}/common_spack.sh
source ${RETURN_PATH}/common_setenv.sh
source ${RETURN_PATH}/common_copy_files.sh
source ${RETURN_PATH}/common_launch_sos.sh
source ${RETURN_PATH}/common_srun_cmds.sh
#
#
####

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
#echo ""
#echo ">>>> Starting SOS daemon statistics monitoring..."
#echo ""
#srun ${SOS_MONITOR_START} &
#
echo ""
echo ">>>> Creating Apollo VIEW and INDEX in the SOS databases..."
echo ""
#
SOS_SQL=${SQL_APOLLO_VIEW} srun ${SRUN_SQL_EXEC}
#SOS_SQL=${SQL_APOLLO_INDEX} srun ${SRUN_SQL_EXEC}
#
echo ""
echo ">>>> Launching experiment codes..."
echo ""
#
export CLEVERLEAF_APOLLO_BINARY=" ${SOS_WORK}/bin/cleverleaf-apollo-release "
export CLEVERLEAF_NORMAL_BINARY=" ${SOS_WORK}/bin/cleverleaf-normal-release "

#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_20.in"
export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_25.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_50.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_100.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_500.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_test.in"

export SRUN_CLEVERLEAF=" "
export SRUN_CLEVERLEAF+=" --cpu-bind=none "
export SRUN_CLEVERLEAF+=" -c 36 "
export SRUN_CLEVERLEAF+=" -N ${WORK_NODE_COUNT} "
export SRUN_CLEVERLEAF+=" -n ${APPLICATION_RANKS} "
export SRUN_CLEVERLEAF+=" -r 1 "

echo ""
echo "========== EXPERIMENTS STARTING =========="
echo ""

function wipe_all_sos_data_from_database() {
    SOS_SQL=${SQL_DELETE_VALS} srun ${SRUN_SQL_EXEC}
    SOS_SQL=${SQL_DELETE_DATA} srun ${SRUN_SQL_EXEC}
    SOS_SQL=${SQL_DELETE_PUBS} srun ${SRUN_SQL_EXEC}
    SOS_SQL="VACUUM;" srun ${SRUN_SQL_EXEC}
}

function run_cleverleaf_with_model() {
    export APOLLO_INIT_MODEL="${SOS_WORK}/$3"
    #wipe_all_sos_data_from_database
    cd output
    printf "\t%4s, %-20s, %-30s, " ${APPLICATION_RANKS} \
        $(basename -- ${CLEVERLEAF_INPUT}) $(basename -- ${APOLLO_INIT_MODEL})
    /usr/bin/time -f %e -- srun ${SRUN_CLEVERLEAF} $1 $2
    cd ${SOS_WORK}
}

##### --- OpenMP Settings ---
# General:
export KMP_WARNINGS="0"
export KMP_AFFINITY="noverbose,nowarnings,norespect,granularity=fine,explicit"
export KMP_AFFINITY="${KMP_AFFINITY},proclist=[0,1,2,3,4,5,6,7,8,9,10,11"
export KMP_AFFINITY="${KMP_AFFINITY},12,13,14,15,16,17,18,19,20,21,22,23"
export KMP_AFFINITY="${KMP_AFFINITY},24,25,26,27,28,29,30,31,32,33,34,35]"
printf "\nKMP_AFFINITY=${KMP_AFFINITY}\n"
##### --- OpenMP Settings ---

set +m
for POLICY_INDEX_0 in $(seq 0 9)
do
    let POLICY_INDEX_1=$[$POLICY_INDEX_0 + 10]
    rm -f ${SOS_WORK}/model.node0
    rm -f ${SOS_WORK}/model.node1
    sed s/SETMODELRULE/${POLICY_INDEX_0}/ ${SOS_WORK}/model.exhaustive.template > ${SOS_WORK}/model.node0
    sed s/SETMODELRULE/${POLICY_INDEX_1}/ ${SOS_WORK}/model.exhaustive.template > ${SOS_WORK}/model.node1
    #run_cleverleaf_with_model ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} "model.current" ${POLICY_INDEX}
    export APOLLO_INIT_MODEL="${SOS_WORK}/model.node0"
    srun ${SRUN_CLEVERLEAF}      -o ${SOS_WORK}/output/cleverleaf.node0.stdout ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} &
    let NODE0_JOB_PID=$!
    export APOLLO_INIT_MODEL="${SOS_WORK}/model.node1"
    srun ${SRUN_CLEVERLEAF} -r 1 -o ${SOS_WORK}/output/cleverleaf.node1.stdout ${CLEVERLEAF_APOLLO_BINARY} ${CLEVERLEAF_INPUT} &
    let NODE1_JOB_PID=$!
    printf "\tRunning  ==>  node0(%2s) @ pid:%s\t\tnode1(%2s) @ pid:%s\n ..." \
        ${POLICY_INDEX_0} ${NODE0_JOB_PID} \
        ${POLICY_INDEX_1} ${NODE1_JOB_PID}
    wait ${NODE0_JOB_PID}
    wait ${NODE1_JOB_PID}
done

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
#####
#
source ${RETURN_PATH}/common_parting.sh
#
set -m
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
sleep 60
echo "--- Done! End of job script. ---"
#
# EOF
