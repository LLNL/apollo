#!/bin/bash -e

###############
# Change these as necessary
###############

TAUDIR=${HOME}/src/tau2/x86_64
SOSDIR=${HOME}/src/sos_flow/build
APOLLODIR=${HOME}/src/apollo

export TAU_MAKEFILE=${TAUDIR}/lib/Makefile.tau-papi-pthread

###############
# The rest shouldn't need to be changed...
###############

# Remove old profiles  and databases

rm -f *.xml sosd.00000.*

# Make new executables
make clean
make

# Set environment to match Quartz/Slurm

export OMP_DISPLAY_ENV=TRUE
export OMP_SCHEDULE=AUTO
export OMP_NUM_THREADS=36
export OMP_PLACES={0:${OMP_NUM_THREADS}:1}
export OMP_PROC_BIND=TRUE
export OMP_STACKSIZE=1024
export OMP_DEFAULT_DEVICE=0

# Start SOS

set +e
killall -9 sosd
set -e
rm -f sosd.00000.db
${SOSDIR}/tmpbin/sosd -l 0 -a 1 -r aggregator -k 0 -w `pwd` &

sleep 3

# Setup stuff

export SLURM_PROCID=12345
export SLURM_NNODES=1
export SLURM_NPROCS=1
export SLURM_CPUS_ON_NODE=36
export SLURM_TASKS_PER_NODE=1
export APOLLO_INIT_MODEL=${APOLLODIR}/jobs/model.static.0.default

# TAU settings

export TAU_EBS_PERIOD=10000
export TAU_PROFILE_FORMAT=merged
export TAU_THROTTLE=0
#export TAU_EBS_UNWIND=1
#export TAU_EBS_UNWIND_DEPTH=3
#export TAU_CALLPATH=1
#export TAU_CALLPATH_DEPTH=100
#export TAU_EBS_RESOLUTION=function
export TAU_EBS_KEEP_UNRESOLVED_ADDR=1
export TAU_SAMPLING=1
#export TAU_METRICS=TIME:PAPI_TOT_INS:PAPI_RES_STL
#export TAU_COMPENSATE=1

tau="tau_exec -T pthread,papi,serial "
time ${tau} ./wave-eqn-normal
mv tauprofile.xml normal-tauprofile.xml

time ${tau} ./wave-eqn-apollo
mv tauprofile.xml apollo-tauprofile.xml
export APOLLO_INIT_MODEL=${APOLLODIR}/jobs/model.static.7.default
time ${tau} ./wave-eqn-apollo
mv tauprofile.xml apollo-7-tauprofile.xml
export APOLLO_INIT_MODEL=${APOLLODIR}/jobs/model.static.13.default
time ${tau} ./wave-eqn-apollo
mv tauprofile.xml apollo-13-tauprofile.xml
export APOLLO_INIT_MODEL=${APOLLODIR}/jobs/model.static.1.default
time ${tau} ./wave-eqn-apollo
mv tauprofile.xml apollo-1-tauprofile.xml
export APOLLO_INIT_MODEL=${APOLLODIR}/jobs/model.roundrobin
time ${tau} ./wave-eqn-apollo
mv tauprofile.xml apollo-rr-tauprofile.xml

# Stop SOS

${SOSDIR}/tmpbin/sosd_stop

