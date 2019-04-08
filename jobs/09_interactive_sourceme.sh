#!/bin/bash

export LAUNCHED_FROM_PATH=`pwd`
export EXPERIMENT_JOB_TITLE="09.lulesh.interactive"
export EXPERIMENT_BASE="/g/g17/wood67/experiments/apollo"

echo ""
echo "Killing all existing 'srun' invocations..."
killall -q srun
echo ""
sleep 2

####
#
#  Launch the SOS runtime:
#
#  Verify the environment has been configured:
source $LAUNCHED_FROM_PATH/common_unsetenv.sh
source $LAUNCHED_FROM_PATH/common_setenv.sh
source $LAUNCHED_FROM_PATH/configure_clean_env.sh
#
export SOS_EVPATH_MEETUP="/g/g17/wood67/experiments/apollo/${EXPERIMENT_JOB_TITLE}.${SLURM_JOB_ID}"
mkdir -p $SOS_WORK
#
# Because we're running interactively, clear this out:
export SOS_BATCH_ENVIRONMENT=""
#
if [ "x$SOS_ENV_SET" == "x" ] ; then
	echo "Please set up your SOS environment first."
    kill -INT $$
fi
if ls $SOS_EVPATH_MEETUP/sosd.*.key 1> /dev/null 2>&1
then
    echo "WARNING: Aggregator KEY and ID file[s] exist already.  Deleting them."
    rm -f $SOS_EVPATH_MEETUP/sosd.*.key
    rm -f $SOS_EVPATH_MEETUP/sosd.*.id
fi
if ls $SOS_WORK/sosd.*.db 1> /dev/null 2>&1
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
echo "    SOS_WORK=$SOS_WORK"
echo ""
#
#
SOS_DAEMON_TOTAL="9"
#
#
srun -N 1 -n 1 -r 0 ${SOS_BUILD_DIR}/bin/sosd -k 0 -r aggregator -l 8 -a 1 -w ${SOS_WORK} & 
echo "   ... aggregator(0) srun submitted."
for LISTENER_RANK in $(seq 1 8)
do
    srun -N 1 -n 1 -r $LISTENER_RANK ${SOS_BUILD_DIR}/bin/sosd -k $LISTENER_RANK -r listener   -l 8 -a 1 -w ${SOS_WORK} &
    echo "   ... listener($LISTENER_RANK) srun submitted."
done
#
#
echo ""
echo "Pausing to ensure runtime is completely established..."
echo ""
SOS_DAEMONS_SPAWNED="0"
while [ $SOS_DAEMONS_SPAWNED -lt $SOS_DAEMON_TOTAL ]
do

    if ls $SOS_WORK/sosd.*.id 1> /dev/null 2>&1
    then
        SOS_DAEMONS_SPAWNED="$(ls -l $SOS_WORK/sosd.*.id | grep -v ^d | wc -l)"
    else
        SOS_DAEMONS_SPAWNED="0"
    fi

    if [ "x$SOS_BATCH_ENVIRONMENT" == "x" ]; then
        for STEP in $(seq 1 20)
        do
            echo -n "  ["
            for DOTS in $(seq 1 $STEP)
            do
                echo -n "#"
            done
            for SPACES in $(seq $STEP 19)
            do
                echo -n " "
            done
            echo -n "]  $SOS_DAEMONS_SPAWNED of $SOS_DAEMON_TOTAL daemons running..."
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
echo "Launching experiment code..."
echo ""
echo "--------------------------------------------------------------------------------"
echo ""
#
#### -------------------------------------------------------------------------
#vvv
#vvv  --- INSERT YOUR EXPERIMENT CODE HERE ---
#vv
#v

# NOTE: Make sure to use the '-r 1' flag on your srun commands
#       if you want to avoid colocating applications with your
#       SOS runtime aggregation daemon, since it is usually
#       busier than regular listeners.
#
# Example:   srun -N 1 -n 8 -r 1 $SOS_BUILD_DIR/bin/demo_app -i 1 -p 5 -m 25
#
export CONTROLLER_COMMAND="-N 1 -n 1 -r 0 ./bin/controller.py"
export JOB_LAUNCH_COMMAND="-N 8 -n 128 -r 1 mpirun ./bin/lulesh-apollo -q -s 100 -i 100 -b 1 -c 1"

# Copy the binary, configuration, and plotting scripts into the folder
# where the output of the job is being stored.
mkdir -p $SOS_WORK/bin
mkdir -p $SOS_WORK/lib
#
cp /g/g17/wood67/src/raja-proxies/build/bin/lulesh-apollo $SOS_WORK/bin
cp /g/g17/wood67/src/raja-proxies/build/bin/lulesh-v2.0-RAJA-seq.exe $SOS_WORK/bin
cp /g/g17/wood67/src/sos_flow/src/python/ssos.py $SOS_WORK/bin
#
cp /g/g17/wood67/src/apollo/install/lib/libapollo.so $SOS_WORK/lib
cp /g/g17/wood67/src/sos_flow/build/lib/libsos.so $SOS_WORK/lib
cp /g/g17/wood67/src/sos_flow/build/lib/ssos_python.so $SOS_WORK/lib
cp /g/g17/wood67/src/caliper/install/lib64/libcaliper.so $SOS_WORK/lib
#
export PYTHONPATH=$SOS_WORK/lib:$PYTHONPATH
#
# Make an archive of this script and the environment config script:
echo "srun $JOB_LAUNCH_COMMAND" > $SOS_WORK/COMMAND_JOB_LAUNCH
echo "srun $CONTROLLER_COMMAND" > $SOS_WORK/COMMAND_CONTROLLER
cp /g/g17/wood67/setenv.sh $SOS_WORK/ENV_USR
cp $LAUNCHED_FROM_PATH/common_setenv.sh $SOS_WORK/ENV_SOS_SET
cp $LAUNCHED_FROM_PATH/common_unsetenv.sh $SOS_WORK/ENV_SOS_UNSET
cp $LAUNCHED_FROM_PATH/configure_clean_env.sh $SOS_WORK/ENV_JOB
cp $BASH_SOURCE $SOS_WORK/SLURM_BASH_SCRIPT

# Go into this location, so that any files created will be stored alongside
# the databases for archival/reproducibility purposes.
cd $SOS_WORK

export CALI_LOG_VERBOSITY=0
export CALI_AGGREGATE_KEY="APOLLO_time_flush"
export CALI_SOS_TRIGGER_ATTR="APOLLO_time_flush"
export CALI_SERVICES_ENABLE="sos,timestamp"
export CALI_TIMER_SNAPSHOT_DURATION="false"
export CALI_SOS_ITER_PER_PUBLISH="1"

srun $JOB_LAUNCH_COMMAND

#
#^
#^^
#^^^
#^^^
####
#
#  Bring the SOS runtime down cleanly:
#
echo "--------------------------------------------------------------------------------"
echo ""
echo "    DONE!"
echo ""
echo "\$SOS_WORK = $SOS_WORK"
echo ""
echo "\$SOS_WORK directory listing:"
tree $SOS_WORK
echo "" > PARTING_INSTRUCTIONS
echo "--------------------------------------------------------------------------------"
echo "The SOS RUNTIME IS STILL UP so you can interactively query / run visualizations." >> PARTING_INSTRUCTIONS
echo "" >> PARTING_INSTRUCTIONS 
echo "You are now in the \$SOS_WORK directory with your RESULTS and SCRIPTS!" >> PARTING_INSTRUCTIONS 
echo "                                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" >> PARTING_INSTRUCTIONS 
echo "" >> PARTING_INSTRUCTIONS 
echo "        To RETURN to your code ..: $ cd \$LAUNCHED_FROM_PATH" >> PARTING_INSTRUCTIONS 
echo "        To SHUT DOWN SOS ........: \$ srun -N 8 -n 8 -r 1 \$SOS_BUILD_DIR/bin/sosd_stop    (OR: \$ killall srun)" >> PARTING_INSTRUCTIONS 
echo "" >> PARTING_INSTRUCTIONS
cat PARTING_INSTRUCTIONS
#
####
