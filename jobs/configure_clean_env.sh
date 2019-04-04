#!/bin/bash
if [ "x$EXPERIMENT_BASE" == "x" ] ; then
    echo "ERROR:   (configure_clean_env.sh)"
	echo "    Please set the EXPERIMENT_BASE before proceeding."
    kill -INT $$
fi
#
#
#
echo ""
echo "batchenv.sh: Configuring your environment for the SOSflow runtime..."
echo "" 
source /g/g17/wood67/src/apollo/experiments/common_unsetenv.sh
source /g/g17/wood67/src/apollo/experiments/common_setenv.sh
export SOS_BATCH_ENVIRONMENT="batch"
echo ""
echo "Setting \$SOS_WORK for this job..."
unset SOS_WORK
if [ "x$EXPERIMENT_JOB_TITLE" == "x" ]
then
    export SOS_WORK=$EXPERIMENT_BASE/job_work/$SLURM_JOBID
else
    export SOS_WORK=$EXPERIMENT_BASE/job_work/$SLURM_JOBID.$EXPERIMENT_JOB_TITLE
fi
rm -f $SOS_WORK
mkdir -p $SOS_WORK
echo ""
env | grep SOS
echo ""
echo "--------------------------------------------------------------------------------"
echo "Your environment should be configured to run SOS experiments!"
echo ""
