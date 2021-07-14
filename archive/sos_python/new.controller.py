#!/usr/bin/env python3

# Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
# Apollo Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import os
import sys
import time
import pickle

from ssos import SSOS

import apollo.trees as trees
import apollo.query as query
import apollo.utils as utils

from apollo.debug  import log
from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT

##########

def main():
    controller_start = time.time()
    SOS = SSOS()
    SOS.init()

    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    step = 0
    prior_frame_max = 0

    log(1, "Online.")
    query.createApolloView(SOS, sos_host, sos_port)

    #log(1, "Wiping all prior data in the SOS database...")
    #query.wipeAllExistingData(SOS, sos_host, sos_port)

    data = {}

    while (os.environ.get("SOS_SHUTDOWN") != "TRUE"):
        # Clearing prior training data from SOS
        # query.wipeTrainingData(SOS, sos_host, sos_port, prior_frame_max)
        data['prior_frame_max'] = \
                query.waitForMoreRowsUsingSQL(SOS, sos_host, sos_port, prior_frame_max)
        data['latest_query_rows'], data['latest_region_names'] = \
                query.getTrainingData(SOS, sos_host, sos_port, row_limit=0);

        pickle_latest_data(cargo)
        dataset_guid = SOS.get_guid()

        # Model: RegressionTree
        data['rtree_skl'] = trees.generateRegressionTree(log, data,
                assign_guid=dataset_guid,
                tree_max_depth=3,
                one_big_tree=False)

        # Model: DecisionTree
        data['dtree_def'], data['dtree_skl'] = \
                trees.generateDecisionTree(
                        log, data, assign_guid=dataset_guid,
                        tree_max_depth=3, one_big_tree=False)

        # TODO(chad): Bootstrap conditions VS. active monitoring conditions

        # Analyze the data coming in compared to existing rtree
        data['model_pkg_json'] = guide.analyzePerformance(data)

        # TODO(chad): Ship out the model package.

        if (ONCE_THEN_EXIT):
            controller_elapsed = time.time() - controller_start
            log(1, "Done.  Full cycle of controller took " \
                    + str(controller_elapsed) + "seconds.")
            return


        step += 1
        ##### return to top of loop until shut down #####


    ########## end of controller.py  ##########
    log(1, "Done.")
    return



#########

def pickle_latest_data(data, step):
    with open(('./output/models/step.%d.pickle' % step), 'wb') as f:
        pickle.dump(data, f)




if __name__ == "__main__":
    main()
