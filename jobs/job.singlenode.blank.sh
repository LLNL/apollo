

# NOTE: This is not done yet...


#!/bin/bash
#
export EXPERIMENT_JOB_TITLE="BLANK.1node.cleverleaf"  # <-- creates output path!
#
export APPLICATION_RANKS="1"         # ^__ make sure to change SBATCH node counts!
export SOS_AGGREGATOR_COUNT="1"      # <-- actual aggregator count
export EXPERIMENT_NODE_COUNT="1"     # <-- is SBATCH -N count, incl/extra agg. node
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
#echo ""
#echo ">>>> Creating Apollo VIEW and INDEX in the SOS databases..."
#echo ""

#SOS_SQL=${SQL_APOLLO_VIEW} srun ${SRUN_SQL_EXEC}
#SOS_SQL=${SQL_APOLLO_INDEX} srun ${SRUN_SQL_EXEC}
#
#
echo ""
echo ">>>> Launching experiment codes..."
echo ""
#

export CLEVERLEAF_APOLLO_BINARY=" ${SOS_WORK}/bin/cleverleaf-apollo-relwithdebinfo "
export CLEVERLEAF_NORMAL_BINARY=" ${SOS_WORK}/bin/cleverleaf-normal-relwithdebinfo "
export CLEVERLEAF_TRACED_BINARY=" ${SOS_WORK}/bin/cleverleaf-normal-relwithdebinfo "

export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_25.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_50.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_100.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_triple_pt_500.in"
#export CLEVERLEAF_INPUT="${SOS_WORK}/cleaf_test.in"

export SRUN_CLEVERLEAF=" "
export SRUN_CLEVERLEAF+=" --cpu-bind=none "
export SRUN_CLEVERLEAF+=" -c 32 "
export SRUN_CLEVERLEAF+=" -o ${SOS_WORK}/output/cleverleaf.%4t.stdout "
export SRUN_CLEVERLEAF+=" -N ${WORK_NODE_COUNT} "
export SRUN_CLEVERLEAF+=" -n ${APPLICATION_RANKS} "
export SRUN_CLEVERLEAF+=" -r 1 "

##### --- OpenMP Settings ---
export KMP_WARNINGS="0"
export KMP_AFFINITY="noverbose,nowarnings,norespect,granularity=fine,explicit"
export KMP_AFFINITY="${KMP_AFFINITY},proclist=[0,1,2,3,4,5,6,7,8,9,10,11"
export KMP_AFFINITY="${KMP_AFFINITY},12,13,14,15,16,17,18,19,20,21,22,23"
export KMP_AFFINITY="${KMP_AFFINITY},24,25,26,27,28,29,30,31,32,33,34,35]"
echo ""
echo "KMP_AFFINITY=${KMP_AFFINITY}"
echo ""
##### --- OpenMP Settings ---

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


export APOLLO_INIT_MODEL=${SOS_WORK}/model.static.0.default
cd ${SOS_WORK}

#####
#
source ${RETURN_PATH}/common_parting.sh
#
echo ""
echo "OK!  You are now ready to run Apollo experiments."
echo ""
#
#####

# EOF
