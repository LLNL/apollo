printf "\n"
for MULTIPLE in $(seq 1 1 14)
do
	let PROC_COUNT=$[$MULTIPLE ** 3]
    # Factor in the aggregator / controller processes:
    let APOLLO_TASKS="2"
    # Start with one work node:
    let NODE_CAPACITY="36"
    let NODE_COUNT="1"
    let REQ_PROC_W_DAEMON=$[$PROC_COUNT + $NODE_COUNT + $APOLLO_TASKS]
    while (($NODE_CAPACITY < $REQ_PROC_W_DAEMON)); do
    	let NODE_CAPACITY=$[$NODE_CAPACITY + 36]
        let NODE_COUNT=$[$NODE_COUNT + 1]
        let REQ_PROC_W_DAEMON=$[$PROC_COUNT + $NODE_COUNT + $APOLLO_TASKS]
    done
    # Add in an extra node for the aggregator / controller:
    let NODE_COUNT=$[$NODE_COUNT + 1]
   	let NODE_CAPACITY=$[$NODE_CAPACITY + 36]
    printf "\t%4d client ranks need %4d nodes and %5d req. processes   (%d total cores)\n" \
        ${PROC_COUNT} ${NODE_COUNT} ${REQ_PROC_W_DAEMON} ${NODE_CAPACITY}
done
printf "\n"
