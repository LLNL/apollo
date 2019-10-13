#  Set up the commands we'll use for this experiment kit:
#
export WORK_NODE_COUNT=$[$EXPERIMENT_NODE_COUNT - 1]
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
echo "srun ${SRUN_CONTROLLER}"       > ${SOS_WORK}/launch/srun_controller
env | grep SLURM                     > ${SOS_WORK}/launch/slurm_env

