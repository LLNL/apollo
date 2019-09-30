#!/bin/bash
if [ "${1}" == "-h" ] ; then
    echo ""
    echo "show.layout.sh -- Display how tasks are assigned by SLURM's srun."
    echo ""
    echo "Usage:"
    echo "    srun -r 2 -N 1 -n 32 --unbuffered -m block ./show.layout.sh"
else
    printf "pid(%s) @ %s  slurm_tasks_per_node: %s  slurm_nprocs: %s  slurm_nnodes: %s\n" \
         ${SLURM_PROCID} ${SLURMD_NODENAME} ${SLURM_TASKS_PER_NODE} ${SLURM_NPROCS} ${SLURM_NNODES}
fi
