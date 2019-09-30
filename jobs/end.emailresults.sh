#!/usr/bin/bash
if [ "x$1" == "x" ] ; then
    echo "USAGE: ./end.emailresults.sh <lulesh | cleverleaf>"
    echo ""
    echo "== end.emailresults.sh: Not emailing a results summary..."
    kill -INT $$
fi
echo ""
echo "== end.emailresults.sh: Compiling results.."
#
if [ "$1" == "cleverleaf" ] ; then
    #
    cd ${SOS_WORK}/output
    zip results.${SLURM_JOB_ID}.summary.zip cleaf*.*.log
    zip results.${SLURM_JOB_ID}.summary.zip cleverleaf.0000.stdout
    zip results.${SLURM_JOB_ID}.summary.zip controller.out
    zip results.${SLURM_JOB_ID}.summary.zip ../daemons/monitor.*.csv
    zip results.${SLURM_JOB_ID}.summary.zip models/*.json
    #
    if [ -d "cleaf*.0.visit" ]; then
        VISIT_DUMP_DIR=$(dirname cleaf*.0.visit/dumps.visit)
        while read summaryfile; do
            zip results.${SLURM_JOB_ID}.summary.zip ${VISIT_DUMP_DIR}/$summaryfile
        done <${VISIT_DUMP_DIR}/dumps.visit
        zip results.${SLURM_JOB_ID}.summary.zip ${VISIT_DUMP_DIR}/dumps.visit
    fi
    echo ""
    echo "== end.emailresults.sh: Mailing results to cdw@cs.uoregon.edu..."
    #
    cat cleverleaf.0000.stdout | mutt -s "Slurm Job_id=${SLURM_JOB_ID} Results Summary" -a results.${SLURM_JOB_ID}.summary.zip -- cdw@cs.uoregon.edu
    #
fi
if [ "$1" == "lulesh" ] ; then
    #
    #  TODO: Add lulesh.
    #
    echo "== end.emailresults.sh: ERROR -- lulesh is not currently supported..."
    #
fi
echo "== end.emailresults.sh: Done."
echo ""
#
