#!/bin/bash
#
export CLEVERLEAF_BINARY="./bin/cleverleaf-apollo-release"
export CLEVERLEAF_INPUT="cleaf_triple_pt_100.in"
#
export APOLLO_COLLECTIVE_TRAINING="1"
export APOLLO_LOCAL_TRAINING="0"
#
export APOLLO_TRACE_ENABLED="true"
export APOLLO_TRACE_EMIT_ONLINE="false"
export APOLLO_TRACE_EMIT_ALL_FEATURES="true"
#
function run_using_policy() {
    export APOLLO_INIT_MODEL="$1,$2"
    export APOLLO_TRACE_OUTPUT_FILE="apollo_trace.$1.$2.csv"
    srun -N 1 -n 1 --ntasks-per-node=1 --mpibind=off ${CLEVERLEAF_BINARY} $PWD/${CLEVERLEAF_INPUT}
}


#testPolicies=("0" "1" "2" "3" "4" "5" "6" "7" "8" "9" "10" "11" "12" "13" "14" "15" "16" "17" "18" "19")

#for polIndex in "${testPolicies[@]}"
#do

#export APOLLO_SINGLE_MODEL="1"
#export APOLLO_REGION_MODEL="0"
#run_using_policy "Static" "0"
#run_using_policy "Static" "1"

export APOLLO_SINGLE_MODEL="0"
export APOLLO_REGION_MODEL="1"
run_using_policy "RoundRobin" "0"

#done
echo "-- done --"
