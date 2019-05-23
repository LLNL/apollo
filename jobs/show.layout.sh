#!/bin/bash
if [ "${1}" == "-h" ] ; then
    echo ""
    echo "show.layout.sh -- Display how tasks are assigned by SLURM's srun."
    echo ""
    echo "Usage:"
    echo "    srun -r 2 -N 1 -n 32 --unbuffered -m block ./show.layout.sh"
else
    printf "slurm_nodename: %s  slurm_procid: %s  slurm_jobid: %s\n" ${SLURMD_NODENAME} ${SLURM_PROCID} ${SLURM_JOBID}
fi
