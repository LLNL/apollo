#!/usr/bin/env python
import os
import json
import time
import cStringIO
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

VERBOSE = False
FRAME_INTERVAL = 200

sos = SSOS() 

# class ControllerState:
#     START      = 'start'
#     WAIT       = 'wait'
#     PROC       = 'proc'
#     TEST       = 'test'
#     ML         = 'ml'
#     RATE       = 'rate'
#     CLIENT_ADD = 'client_add'
#     CLIENT_REM = 'client_rem'
#     END        = 'end'
#     #
#     STATES = frozenset([START, WAIT, PROC, TEST, ML, RATE, CLIENT_ADD, CLIENT_REM, END])
#     at = String(choices=STATES)
#     
#     def __init__(self):
#         self.at = self.START
#     #
#     ### end: class ControllerState

def main():


    sos.init()
    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    # state = ControllerState()
    # while state.at != state.END:
    # ########## main loop ##########

    #     if state.at == state.START:
    #         #
    #     elif state.at == state.WAIT:
    #         #
    #     elif state.at == state.PROC:
    #         #
    #     elif state.at == state.TEST:
    #         #
    #     elif state.at == state.ML:
    #         #
    #     elif state.at == state.RATE:
    #         #
    #     elif state.at == state.CLIENT_ADD:
    #         #
    #     elif state.at == state.CLIENT_REM:
    #         #
    #     else
    #         #

    step = 0
    prior_frame_max = 0

    while (True):
        prior_frame_max = waitForMoreRows(sos_host, sos_port, prior_frame_max)

        # Submit the query to SOS, return results as a Pandas DataFrame:
        data, region_names = getTrainingData(sos_host, sos_port, row_limit=0);
    
        # Process an example ML pipeline w/hardcoded field names:
        model_def = ""
        model_len = 0
    
        model_def, rules_code = learningExample(data, region_names)
        model_len = len(model_def)
    
        print "-----"
        print "STEP " + str(step) + " RULES:"
        print rules_code
        print "-----"
    
        if model_len > 0:
            if (VERBOSE):
                print "== CONTROLLER:  Sending model to SOS runtime for distribution to Apollo..."
                print model_def
            sos.trigger("APOLLO_MODELS", model_len, model_def)
        else:
            if (VERBOSE):
                print "== CONTROLLER:  NOTICE: Model was not generated, nothing to send."
        #
        step += 1

    ########## end main loop ##########
    if (VERBOSE):
        print "== CONTROLLER:  Done."
    return

#########

def waitForMoreRows(sos_host, sos_port, prior_frame_max):
    max_frame, results, col_names = \
            sos.request_pub_manifest("", sos_host, sos_port)
    while (max_frame < (prior_frame_max + FRAME_INTERVAL)):
        print "== CONTROLLER:  Waiting for more data.  (" \
            + str(max_frame - prior_frame_max) + " of " \
            + str(FRAME_INTERVAL) + " new frames, " + str(max_frame) \
            + " total)"
        time.sleep(1)
        max_frame, results, col_names = \
            sos.request_pub_manifest("", sos_host, sos_port)
    print "== CONTROLLER: max_frame = " + str(max_frame)
    return max_frame

def tablePrint(results):
    # Print out the results in a pretty column-aligned way:
    widths = [max(map(len, str(col))) for col in zip(*results)]
    for row in results: 
        print "  ".join((val.ljust(width) for val, width in zip(row, widths)))
    #
    return



def getTrainingData(sos_host, sos_port, row_limit):

    # Drive an outer loop that grabs the list of unique loop names
    sql_string = """
        SELECT DISTINCT loop FROM (
            SELECT
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.end"
                                  THEN tblVals.val END) AS "cali_event_end", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "loop"
                                  THEN tblVals.val END) AS "loop" 
            FROM   tblPubs 
                  LEFT OUTER JOIN tblData 
                               ON tblPubs.guid = tblData.pub_guid 
                  LEFT OUTER JOIN tblVals 
                               ON tblData.guid = tblVals.guid 
            GROUP BY
                tblVals.meta_relation_id,
                tblPubs.guid
            ) WHERE  cali_event_end IS NOT NULL;
    """

    if (VERBOSE):
        print "== CONTROLLER:  Retrieving list of unique loops being measured."
    region_names, col_names = sos.query(sql_string, sos_host, sos_port) 
    if (VERBOSE):
        print "== CONTROLLER:  Loop name list received:"
        for row in region_names:
            print "    " + str(row[0])

    sql_string = """
        SELECT DISTINCT name FROM tblData;
    """
    if (VERBOSE):
        print "== CONTROLLER:  Retrieving possible field names."
    fields_avail, col_names = sos.query(sql_string, sos_host, sos_port)
    if (VERBOSE):
        print "== CONTROLLER:  Field names available for query:"
        for row in fields_avail:
            print "    " + str(row[0])

    sql_string = """
        SELECT frame, loop, policyIndex, op_count, t_total, t_noise, (t_total / op_count) AS t_op_avg FROM (
            SELECT
                  tblVals.frame AS frame,
                  tblData.guid AS guid,
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "loop"
                                  THEN tblVals.val END) AS "loop", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "policyIndex"
                                  THEN tblVals.val END) AS "policyIndex", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "op_count"
                                  THEN tblVals.val END) AS "op_count",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "t_total"
                                  THEN tblVals.val END) AS "t_total", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "t_noise"
                                  THEN tblVals.val END) AS "t_noise",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "sum#time.inclusive.duration"
                                  THEN tblVals.val END) AS "sum_time_inclusive_duration",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali_event_end"
                                  THEN tblVals.val END) AS "cali_event_end"
            FROM   tblPubs 
                  LEFT OUTER JOIN tblData 
                               ON tblPubs.guid = tblData.pub_guid 
                  LEFT OUTER JOIN tblVals 
                               ON tblData.guid = tblVals.guid 
            GROUP BY
                tblVals.meta_relation_id,
                tblPubs.guid
        )   WHERE  cali_event_end IS NOT NULL

        ORDER BY
            loop
    """

    if (row_limit < 1):
        sql_string += ";"
    else:
        sql_string += "LIMIT " + str(row_limit) + ";"

    if (VERBOSE):
        print "== CONTROLLER:  Sending a query for training data..."
    results, col_names = sos.query(sql_string, sos_host, sos_port)

    if (VERBOSE):
        print col_names
        tablePrint(results)

    data = pd.DataFrame.from_records(results, columns=col_names)

    if (VERBOSE):
        print "== CONTROLLER:  Received training data from SOS..."

    return data, region_names


def learningExample(data, region_names): 
    #print "numpy.__version__   == " + str(np.__version__)
    #print "pandas.__version__  == " + str(pd.__version__)
    #print "sklearn.__version__ == " + str(skl.__version__)

    if (VERBOSE):
        print "== CONTROLLER:  Extracting dependent variable for Y-axis..."
    #y = (data["t_op_avg"].values.astype(float)/5).astype(int)
    y = data["t_op_avg"].astype(int)
    y = y.squeeze()

    # From among: frame, loop, policyIndex, op_count, t_total, t_noise
    if (VERBOSE):
        print "== CONTROLLER:  Exclude columns we don't want to use for learning..."
    x = data.drop([
            "frame",
            "loop",
            "t_total",
            "t_op_avg",
            "policyIndex"
        ], axis="columns").values.astype(float)

    if (VERBOSE):
        print "== CONTROLLER:  Initializing pipelines..."
    basic_pipe = [('estimator',   DecisionTreeClassifier(max_depth=2))]
    #dtree_pipe = [('standardize', StandardScaler()),\
    dtree_pipe = [\
                  ('estimator',   DecisionTreeClassifier(
                      class_weight=None, criterion='gini', max_depth=2,
                      max_features=1, max_leaf_nodes=None,
                      min_impurity_split=1e-07, min_samples_leaf=1,
                      min_samples_split=2, min_weight_fraction_leaf=0.0,
                      presort=False, random_state=None, splitter='best'))]
    rtree_pipe = [('estimator',   DecisionTreeRegressor(max_depth=2))]
    svc_pipe =   [('standardize', StandardScaler()),\
                 ('estimator',    SVC())]

    if (VERBOSE):
        print "== CONTROLLER:  Selecting pipe..."
    #pipe = basic_pipe
    #pipe = rtree_pipe
    pipe = dtree_pipe
    #pipe = svc_pipe

    if (VERBOSE):
        print "== CONTROLLER:  Initializing model..."
    model = Pipeline(pipe)

    #if (VERBOSE): print "== CONTROLLER:  Cross-validation... (10-fold)"
    #scores = cross_val_score(model, x, y, cv=10)
    #print("\n".join([("    " + str(score)) for score in scores]))
    #print "    score.mean == " + str(np.mean(scores))

    if (VERBOSE):
        print "== CONTROLLER:  Training model..."
    model.fit(x, y)

    trained_model = model.named_steps['estimator']

    if (VERBOSE):
        print "== CONTROLLER:  Encoding rules..."
    #
    feature_names = ["op_count", "t_noise"]
    #
    rules_json = tree_to_json(trained_model, feature_names)
    rules_code = tree_to_code(trained_model, feature_names)
    #rules_code = tree_to_string(trained_model, feature_names)

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['index'] = 3
    model_def['type']['name'] = "DecisionTree"
    model_def['region_names'] = region_names
    model_def['features'] = {}
    model_def['features']['count'] = len(feature_names)
    model_def['features']['names'] = []
    model_def['features']['names'] = feature_names
    model_def['driver'] = {}
    model_def['driver']['format'] = "json"
    model_def['driver']['rules'] = rules_json 

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4)

    return model_as_json, rules_code

 

#########

from sklearn.tree import _tree

def tree_to_json(decision_tree, feature_names=None):
    from warnings import warn
 
    js = ""
 
    def node_to_str(tree, node_id, criterion):
        if not isinstance(criterion, skl.tree.tree.six.string_types):
            criterion = "impurity"
 
        value = tree.value[node_id]
        if tree.n_outputs == 1:
            value = value[0, :]
 
        jsonValue = ', '.join([str(x) for x in value])
 
        if tree.children_left[node_id] == skl.tree._tree.TREE_LEAF:
            return '"id": "%s", "criterion": "%s", "impurity": "%s", "samples": "%s", "value": [%s]' \
                         % (node_id, 
                                criterion,
                                tree.impurity[node_id],
                                tree.n_node_samples[node_id],
                                jsonValue)
        else:
            if feature_names is not None:
                feature = feature_names[tree.feature[node_id]]
            else:
                feature = tree.feature[node_id]
 
            if "=" in feature:
                ruleType = "="
                ruleValue = "false"
            else:
                ruleType = "<="
                ruleValue = "%.4f" % tree.threshold[node_id]
 
            return '"id": "%s", "rule": "%s %s %s", "%s": "%s", "samples": "%s"' \
                         % (node_id, 
                                feature,
                                ruleType,
                                ruleValue,
                                criterion,
                                tree.impurity[node_id],
                                tree.n_node_samples[node_id])
 
    def recurse(tree, node_id, criterion, parent=None, depth=0):
        tabs = "    " * depth
        js = ""
 
        left_child = tree.children_left[node_id]
        right_child = tree.children_right[node_id]
 
        js = js + "\n" + \
                 tabs + "{\n" + \
                 tabs + "    " + node_to_str(tree, node_id, criterion)
 
        if left_child != skl.tree._tree.TREE_LEAF:
            js = js + ",\n" + \
                     tabs + '    "left": ' + \
                     recurse(tree, \
                                     left_child, \
                                     criterion=criterion, \
                                     parent=node_id, \
                                     depth=depth + 1) + ",\n" + \
                     tabs + '    "right": ' + \
                     recurse(tree, \
                                     right_child, \
                                     criterion=criterion, \
                                     parent=node_id,
                                     depth=depth + 1)
 
        js = js + tabs + "\n" + \
                 tabs + "}"
 
        return js
 
    if isinstance(decision_tree, skl.tree.tree.Tree):
        js = js + recurse(decision_tree, 0, criterion="impurity")
    else:
        js = js + recurse(decision_tree.tree_, 0, criterion=decision_tree.criterion)
 
    return js



def tree_to_string(tree, feature_names):
    result = cStringIO.StringIO()
    tree_ = tree.tree_
    feature_name = [
        feature_names[i] if i != _tree.TREE_UNDEFINED else "undefined!"
        for i in tree_.feature ]
    #Begin recursively encoding the decision tree:
    def recurseSTR(result_str, node, depth):
        offset = "    " * depth
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            result_str.write("{} {}{} <= {}\n".format(depth, offset, name, threshold))
            recurseSTR(result_str, tree_.children_left[node], depth + 1)
            result_str.write("{} {}{} > {}\n".format(depth, offset, name, threshold))
            recurseSTR(result_str, tree_.children_right[node], depth + 1)
        else:
            #result_str.write("INDEX EQ {}\n".format(tree_.value[node]))
            result_val = tree_.value[node]
            result_str.write("{} {}result = {}\n".format(depth, offset, result_val))
    recurseSTR(result, 0, 1)
    return result.getvalue()


def tree_to_code(tree, feature_names):
    tree_ = tree.tree_
    feature_name = [feature_names[i] if i != _tree.TREE_UNDEFINED else "undefined!"
        for i in tree_.feature]
    result = cStringIO.StringIO()
    result.write("def tree({}):\n".format(", ".join(feature_names)))

    def recurse(result_str, node, depth):
        indent = "    " * depth
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            result_str.write("{}if {} <= {}:\n".format(indent, name, threshold))
            recurse(result_str, tree_.children_left[node], depth + 1)
            result_str.write("{}else:  # if {} > {}\n".format(indent, name, threshold))
            recurse(result_str, tree_.children_right[node], depth + 1)
        else:
            result_str.write("{}return {}\n".format(indent, tree_.value[node]))

    recurse(result, 0, 1)
    return result.getvalue()


if __name__ == "__main__":
    main()
