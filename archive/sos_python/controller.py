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

from apollo.debug import log
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
    print("CREATE VIEW", flush=True) # ggout
    query.createApolloView(SOS, sos_host, sos_port)

    #log(1, "Wiping all prior data in the SOS database...")
    print("WIPE DATA", flush=True) # ggout
    query.wipeAllExistingData(SOS, sos_host, sos_port)
    prev_dtree_def = None

    triggers = 0
    while (os.environ.get("SOS_SHUTDOWN") != "TRUE"):
        # Clearing prior training data
        # query.wipeTrainingData(SOS, sos_host, sos_port, prior_frame_max)
        prior_frame_max    = query.waitForMoreRowsUsingSQL(
                                SOS, sos_host, sos_port,
                                prior_frame_max)
        data, region_names = query.getTrainingData(SOS, sos_host, sos_port, row_limit=0);
        #print('data', data)
        #print('region_names', region_names)
        dataset_guid = SOS.get_guid()
        data.to_pickle("./output/models/step.%d.trainingdata.pickle" % prior_frame_max)
        with open(("./output/models/step.%d.region_names.pickle" % prior_frame_max), "wb") as f:
            pickle.dump(region_names, f)
        print("Pickled step ",prior_frame_max) # ggout
        continue; # ggout

        # Model: DecisionTree
        dtree_def, dtree_skl = trees.generateDecisionTree(log, data,
                assign_guid=dataset_guid,
                tree_max_depth=3,
                one_big_tree=False)
        dtree_len = len(dtree_def)

        if (dtree_len < 1):
            log(0, "No models generated for step %d." % prior_frame_max)
        else:
            with open(("./output/models/step.%d.model.json" % prior_frame_max), "w") as f:
               f.write(dtree_def)

        if True:#prev_dtree_def == None or prev_dtree_def != dtree_def:
            prev_dtree_def = dtree_def
            #SOS.trigger("APOLLO_MODELS", dtree_len, dtree_def)
            triggers += 1
            print("===> Trigger ", triggers, " because models differ", flush=True) # ggout

        # Model: RegressionTree
        #rtree_skl = trees.generateRegressionTree(log, data,
        #        assign_guid=dataset_guid,
        #        tree_max_depth=3,
        #        one_big_tree=False)

        # TODO(chad): Add NN models / streaming models here

        # TODO(chad): Drop models into an arena to fight, and only send models
        #             out when they are better than some prior model for any
        #             given loop. Could use async queues for analysis and for
        #             model distribution.

        if dtree_len > 0:
            if (ONCE_THEN_EXIT):
                controller_elapsed = time.time() - controller_start
                log(1, "Done.  Full cycle of controller took "
                       + str(controller_elapsed) + "seconds.")
                return
        else:
            if (VERBOSE):
                log(1, "NOTICE: Model was not generated, nothing to send.")
            if (ONCE_THEN_EXIT):
                log(1, "Done.")
                return

        step += 1
        ##### return to top of loop until shut down #####


    ########## end of controller.py  ##########
    log(1, "Done.")
    return

#########



if __name__ == "__main__":
    main()
