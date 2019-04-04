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
source $EXPERIMENT_BASE/configure_clean_env.sh
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
    echo "WARNING: Aggregator KEY file[s] exist already.  Deleting them."
    rm -f $SOS_EVPATH_MEETUP/sosd.*.key
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

    if ls $SOS_WORK/sosd.*.db 1> /dev/null 2>&1
    then
        SOS_DAEMONS_SPAWNED="$(ls -l $SOS_WORK/sosd.*.db | grep -v ^d | wc -l)"
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

# Copy the binary, configuration, and plotting scripts into the folder
# where the output of the job is being stored.
#TODO: export JOB_LAUNCH_COMMAND="-N 8 -n 8 -r 1 ./lulesh-apollo -s 100"
#TODO: export JOB_BINARY_PATH=$PROJECT_BASE/sos_vpa_2017/chad/lulesh2.0.3
#TODO: cp $JOB_BINARY_PATH/lulesh_par                    $SOS_WORK
#TODO: cp $JOB_BINARY_PATH/alpine_options.json           $SOS_WORK
#TODO: cp $JOB_BINARY_PATH/alpine_actions.json           $SOS_WORK
#TODO: cp $PROJECT_BASE/matts_python_stuff/ssos.py       $SOS_WORK
#TODO: cp $PROJECT_BASE/sos_flow/python/ssos_python.o    $SOS_WORK
#TODO: cp $PROJECT_BASE/sos_flow/python/ssos_python.so   $SOS_WORK
#TODO: cp $PROJECT_BASE/sos_flow/python/plot_lulesh.py   $SOS_WORK
#TODO: cp $PROJECT_BASE/matts_python_stuff/visitlog.py   $SOS_WORK
#TODO: cp $PROJECT_BASE/matts_python_stuff/vtk_writer.py $SOS_WORK


# Make an archive of this script and the environment config script:
#TODO: echo "srun $JOB_LAUNCH_COMMAND" > $SOS_WORK/LAUNCH_COMMAND
#TODO: cp $PROJECT_BASE/job_scripts/batchenv.sh $SOS_WORK
#TODO: cp $BASH_SOURCE $SOS_WORK/LAUNCH_SCRIPT


# Go into this location, so that any files created will be stored alongside
# the databases for archival/reproducibility purposes.
cd $SOS_WORK


#TODO: srun $JOB_LAUNCH_COMMAND
export JOB_LAUNCH_COMMAND="-N 8 -n 8 -r 1 $SOS_BUILD_DIR/bin/demo_app -i 1 -p 100 -m 1000000 -c 0"
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
