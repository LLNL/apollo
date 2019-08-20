#!/usr/bin/env python
import os
import sys
import json
import time
import io
import warnings
import numpy   as np
import pandas  as pd
import sklearn as skl
from sklearn.preprocessing   import StandardScaler
from sklearn.tree            import DecisionTreeClassifier
from sklearn.tree            import DecisionTreeRegressor
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.svm             import SVC

from ssos import SSOS

from apollo.debug import log
from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT

import apollo.utils


def getTrainingData(SOS, sos_host, sos_port, row_limit):
    #####
    #
    #  NOTE: We can split the results up in Pandas to train each region
    #        independently, we don't need to submit that as multiple queries
    #        just yet.
    #
    #        We do grab the list of region names so we can append it to
    #        the encoding of the model, for now.
    #
    log(2, "Submitting SQL queries ...")
    sql_string = "SELECT DISTINCT region_name FROM viewApollo WHERE region_name IS NOT NULL;"
    names_start = time.time()
    region_name_list, col_names = SOS.query(sql_string, sos_host, sos_port)
    names_elapsed = time.time() - names_start
    log(2, "Apollo::Region list (" + str(len(region_name_list))\
                + " x " + str(len(col_names)) + ") retrieved in " + str(names_elapsed)\
                + " seconds.")
    region_names = []
    for nm in region_name_list:
        region_names.append(str(nm[0]))

    ###
    #
    #  Now get the full training set:
    #
    sql_string = """\
        SELECT *
        FROM   viewApollo
        WHERE  region_name IS NOT NULL
        """

    if (row_limit < 1):
        sql_string += ";"
    else:
        sql_string += "LIMIT " + str(row_limit) + ";"

    view_start = time.time()
    results, col_names = SOS.query(sql_string, sos_host, sos_port)
    view_elapsed = time.time() - view_start
    log(2, "viewApollo data (" + str(len(results))\
                    + " x " + str(len(col_names)) + ") retrieved in " + str(names_elapsed)\
                    + " seconds.")

    convert_start = time.time()
    data = pd.DataFrame.from_records(results, columns=col_names)
    convert_elapsed = time.time() - convert_start
    log(2, "Converted to DataFrame in " + str(convert_elapsed) + " seconds.")


    return data, region_names



def wipeAllExistingData(SOS, sos_host, sos_port):
    sql_string =  "DELETE FROM tblVals;"
    region_names, col_names = SOS.query(sql_string, sos_host, sos_port)
    sql_string =  "DELETE FROM tblData;"
    region_names, col_names = SOS.query(sql_string, sos_host, sos_port)
    sql_string =  "DELETE FROM tblPubs;"
    region_names, col_names = SOS.query(sql_string, sos_host, sos_port)
    return

def wipeTrainingData(SOS, sos_host, sos_port, prior_frame_max):
    sql_string =  "DELETE FROM tblVals "
    sql_string += "WHERE tblVals.frame < " + str(prior_frame_max) + ";"
    region_names, col_names = SOS.query(sql_string, sos_host, sos_port)
    return



def checkLatestFrameUsingSQL(SOS, sos_host, sos_port, prior_frame_max):
    results, col_names = \
            SOS.query("SELECT max(latest_frame) FROM tblPubs;", sos_host, sos_port)
    if len(results) < 1:
        return 0
    else:
        if results[0][0] == "NULL":
            return 0
        else:
            return int(results[0][0])

def waitForMoreRowsUsingSQL(SOS, sos_host, sos_port, prior_frame_max):
    max_frame = checkLatestFrameUsingSQL(SOS, sos_host, sos_port, prior_frame_max)
    log(2, "Waiting for %d frames of new data..." % FRAME_INTERVAL)
    while(max_frame < (prior_frame_max + FRAME_INTERVAL)):
        time.sleep(1)
        max_frame = checkLatestFrameUsingSQL(SOS, sos_host, sos_port, prior_frame_max)
    log(2, "Enough frames have arrived at SOS.  max_frame == %d" % max_frame)
    return max_frame


def waitForMoreRows(SOS, sos_host, sos_port, prior_frame_max):
    if (ONCE_THEN_EXIT):
        log(2, "Using ONCE_THEN_EXIT mode, not waiting for new rows.")
        return prior_frame_max

    log(2, "Waiting for data...")

    max_frame, results, col_names = \
            SOS.request_pub_manifest("", sos_host, sos_port)

    while (max_frame < (prior_frame_max + FRAME_INTERVAL)):
        time.sleep(1)
        max_frame, results, col_names = \
            SOS.request_pub_manifest("", sos_host, sos_port)

    #####
    return max_frame



def createApolloView(SOS, sos_host, sos_port):
    sql_cmd = """
	CREATE VIEW IF NOT EXISTS viewApollo AS
            SELECT
                  tblVals.frame AS frame,
                  tblPubs.comm_rank AS mpi_rank,
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "region_name"
                                  THEN tblVals.val END) AS "region_name",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "policy_index"
                                  THEN CAST(tblVals.val AS INTEGER) END) AS "policy_index",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "step"
                                  THEN CAST(tblVals.val AS INTEGER) END) AS "step",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "num_elements"
                                  THEN CAST(tblVals.val AS INTEGER) END) AS "num_elements",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "exec_count"
                                  THEN tblVals.val END) AS "exec_count",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "time_last"
                                  THEN tblVals.val END) AS "time_last",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "time_min"
                                  THEN tblVals.val END) AS "time_min",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "time_max"
                                  THEN tblVals.val END) AS "time_max",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "time_avg"
                                  THEN tblVals.val END) AS "time_avg"
            FROM   tblPubs
                  LEFT OUTER JOIN tblData
                               ON tblPubs.guid = tblData.pub_guid
                  LEFT OUTER JOIN tblVals
                               ON tblData.guid = tblVals.guid
            GROUP BY
                tblPubs.guid,
                tblVals.meta_relation_id
	;
    """
    ret_rows, ret_cols = SOS.query(sql_cmd, sos_host, sos_port)
    return


