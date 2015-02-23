#!/bin/bash
#MSUB -l nodes=1
#MSUB -l partition=rzmerl
#MSUB -l walltime=01:30:00
#MSUB -q pbatch
#MSUB -V

# Usage: ./lulesh-SEGIT_OMP-SEG_OMP [opts]
# where [opts] is one or more of:
# -q              : quiet mode - suppress all stdout
# -i <iterations> : number of cycles to run
# -s <size>       : length of cube mesh along side
# -r <numregions> : Number of distinct regions (def: 11)
# -b <balance>    : Load balance between regions of a domain (def: 1)
# -c <cost>       : Extra cost of more expensive regions (def: 1)
# -f <numfiles>   : Number of files to split viz dump into (def: (np+10)/9)
# -p              : Print out progress
# -v              : Output viz file (requires compiling with -DVIZ_MESH
# -h              : This message

export OMP_NUM_THREADS=16

for j in SEGIT_SEQ SEGIT_OMP SEGIT_CILK; do
  for k in SEG_SEQ SEG_SIMD SEG_OMP SEG_CILK; do
    for l in 5 10 15 20 25 30 50 75 100; do
      ../lulesh-${j}-${k} -q -i 15 -s ${l} > ${j}-${k}-${l}.out
    done
  done
done
