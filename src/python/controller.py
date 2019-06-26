#!/usr/bin/env python
import os
import sys
import time

from ssos import SSOS

import apollo.trees as trees
import apollo.query as query
import apollo.utils as utils

from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT

##########

def main():

    SOS = SSOS()
    SOS.init()

    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    step = 0
    prior_frame_max = 0

    if (VERBOSE): print "== CONTROLLER:  Online."
    if (VERBOSE): print "== CONTROLLER:  CREATE VIEW IF NOT EXIST viewApollo ..."
    query.createApolloView(SOS, sos_host, sos_port)

    #print ("== CONTROLLER: wipeTrainingData")
    #query.wipeTrainingData(SOS, sos_host, sos_port, prior_frame_max)
    while (os.environ.get("SOS_SHUTDOWN") != "TRUE"):
        prior_frame_max    = query.waitForMoreRows(SOS, sos_host, sos_port, prior_frame_max)
        data, region_names = query.getTrainingData(SOS, sos_host, sos_port, row_limit=0);

        model_def = ""
        model_len = 0

        # DECISIONTREE
        print "== CONTROLLER:  Generating DecisionTree ... "
        model_def, rules_code = trees.generateDecisionTree(SOS, data, region_names)
        model_len = len(model_def)
        if (VERBOSE):
            print "----------"
            #print "== CONTROLLER:  model_def ="
            #print model_def
            print "== CONTROLLER:  rules_code ="
            print rules_code
            print "----------"

        # REGRESSIONTREE
        #model_def = trees.generateRegressionTree(SOS, data, region_names)
        #model_len = len(model_def)

        # STATIC
        #model_def = utils.generateStaticModel(SOS, data, region_names)
        #model_len = len(model_def)

        if model_len > 0:
            if (VERBOSE):
                print "== CONTROLLER:  Sending >>> DECISIONTREE <<< to SOS for Apollo..."
                print "== CONTROLLER:    ..."
            SOS.trigger("APOLLO_MODELS", model_len, model_def)

            if (ONCE_THEN_EXIT):
                print "== CONTROLLER:  Done."
                return

            if (VERBOSE): print "== CONTROLLER:  Pausing to allow new model to run for a fresh interval ..."
            query.waitForMoreRows(SOS, sos_host, sos_port, prior_frame_max);

            #if (VERBOSE): print "== CONTROLLER:  Clearing prior training data..."
            #query.wipeTrainingData(SOS, sos_host, sos_port, prior_frame_max)

            if (VERBOSE):
                print "== CONTROLLER:  Sending >>> RANDOMSEARCH <<< to SOS for Apollo..."
                print "== CONTROLLER:    ..."
            model_def = utils.generateRandomModel(SOS, data, region_names)
            model_len = len(model_def)
            SOS.trigger("APOLLO_MODELS", model_len, model_def)

            prior_frame_max, pub_titles, col_names = \
                SOS.request_pub_manifest("", sos_host, sos_port)
        else:
            if (VERBOSE):
                print "== CONTROLLER:  NOTICE: Model was not generated, nothing to send."
            if (ONCE_THEN_EXIT):
                print "== CONTROLLER:  Done."
                return
        #
        step += 1

    ########## end main loop ##########
    if (VERBOSE):
        print "== CONTROLLER:  Done."
    return

#########



if __name__ == "__main__":
    main()
