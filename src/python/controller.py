#!/usr/bin/env python

# Copyright (c) 2019, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory
#
# This file is part of Apollo.
# OCEC-17-092
# All rights reserved.
#
# Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
# of many collaborators.
#
# Apollo was originally created by David Beckingsale, david@llnl.gov
#
# For details, see https://github.com/LLNL/apollo.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


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
    query.createApolloView(SOS, sos_host, sos_port)

    #log(1, "Wiping all prior data in the SOS database...")
    #query.wipeAllExistingData(SOS, sos_host, sos_port)

    while (os.environ.get("SOS_SHUTDOWN") != "TRUE"):
        prior_frame_max    = query.waitForMoreRowsUsingSQL(
                                SOS, sos_host, sos_port,
                                prior_frame_max)
        data, region_names = query.getTrainingData(SOS, sos_host, sos_port, row_limit=0);
        data.to_pickle("./output/models/step.%d.trainingdata.pickle" % prior_frame_max)
        with open(("./output/models/step.%d.region_names.pickle" % prior_frame_max), "w") as f:
            pickle.dump(region_names, f)

        model_def = ""
        model_len = 0

        # DECISIONTREE
        model_def = trees.generateDecisionTree(SOS, data, region_names)
        model_len = len(model_def)

        # REGRESSIONTREE
        #model_def = trees.generateRegressionTree(SOS, data, region_names)
        #model_len = len(model_def)

        if model_len > 0:
            trigger_start = time.time()
            SOS.trigger("APOLLO_MODELS", model_len, model_def)
            trigger_elapsed = time.time() - trigger_start
            log(1, "Sent models to SOS for Apollo in " + str(trigger_elapsed) + " seconds.")
            log(1, "Writing models to \"prev_model.json\" ...")

            with open(("./output/models/step.%d.decisiontree.json" % prior_frame_max), "w") as mf:
                mf.write(model_def)
            with open("./output/models/latest_model.json", "w") as mf:
                mf.write(model_def)

            if (ONCE_THEN_EXIT):
                controller_elapsed = time.time() - controller_start
                log(1, "Done.  Full cycle of controller took " + str(controller_elapsed) + "seconds.")
                return

            #if (VERBOSE): print "== CONTROLLER:  Pausing to allow new model to run for a fresh interval ..."
            #query.waitForMoreRows(SOS, sos_host, sos_port, prior_frame_max);

            #log(1, "Clearing prior training data...")
            #query.wipeTrainingData(SOS, sos_host, sos_port, prior_frame_max)

            #trigger_start = time.time()
            #SOS.trigger("APOLLO_MODELS", model_len, model_def)
            #trigger_elapsed = time.time() - trigger_start
            #if (VERBOSE):
            #    print "== CONTROLLER:  Sent models to SOS for Apollo in " + str(trigger_elapsed) + " seconds."

            #prior_frame_max, pub_titles, col_names = \
            #    SOS.request_pub_manifest("", sos_host, sos_port)
        else:
            if (VERBOSE):
                log(1, "NOTICE: Model was not generated, nothing to send.")
            if (ONCE_THEN_EXIT):
                log(1, "Done.")
                return
        #
        step += 1

    ########## end main loop ##########
    log(1, "Done.")
    return

#########



if __name__ == "__main__":
    main()
