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

VERBOSE = True
FRAME_INTERVAL = 200
SOS = SSOS() 

def generateStaticModel(data, region_names):

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['index'] = 3
    model_def['type']['name'] = "Static"
    model_def['region_names'] = []
    for n in region_names:
        for nm in n:
            model_def['region_names'].append(nm)
    model_def['features'] = {}
    model_def['features']['count'] = 0
    model_def['features']['names'] = [] 
    model_def['driver'] = {}
    model_def['driver']['format'] = "int"
    model_def['driver']['rules'] = "1" 
    
    model_as_json = json.dumps(model_def, sort_keys=False, indent=4)

    if VERBOSE:
        print "== CONTROLLER: model_as_json = " + model_as_json

    return model_as_json


def generateSyntheticDecisionTree(data, region_names):

    fake_json = {}
    fake_json = {} 
    fake_json['type'] = {}
    fake_json['type']['index'] = 4 
    fake_json['type']['name'] = "DecisionTree"
    fake_json['region_names'] = []
    fake_json['region_names'] = region_names
    fake_json['features'] = {}
    fake_json['features']['count'] = len(feature_names)
    fake_json['features']['names'] = []
    fake_json['features']['names'] = feature_names
    fake_json['driver'] = {}
    fake_json['driver']['format'] = "json"
    fake_json['driver']['rules'] = rules_json 



    return fake_json


def generateDecisionTree(data, region_names): 
    #print "numpy.__version__   == " + str(np.__version__)
    #print "pandas.__version__  == " + str(pd.__version__)
    #print "sklearn.__version__ == " + str(skl.__version__)

    # TODO: Label the data (mark the winning KernelVariant as 1, else 0)
    #
    # If (iteration % 10) is some "GroupID" then we find the fastest variant
    # for that group. Maybe make the operation weight a function of iteration
    # within that GroupId. This lets us pretend different iteration groupings 
    # have different behavior.

    #sort by t_op_avg
    #group by group_id
    #drop slowest ones (or take first from each group, Pandas-style)

    # NOTE: The rows that are dropped should be identical to the rows
    # that are kept EXCEPT for the time and the variant.


    #y = target value
    #x = independent features (data that would effect the kernel decision)

    #x is not something dependent on the kernel that was selected.
    #x determines y
    # example: X will not be t_total


# Prepare the data before building the tree:
#       filter out all but the best scoring t_totals
#       for every unique combination of X values
#   [Y]    [ X:                             ]    -cut- (used to determine Y)
#   kern   [ t_op    op_count    t_op_weight]     t_total
#    0     [  2        1212           1     ]       112
#    1     [  4        1212           1     ]       112
#    2     [  2        1212           1     ]       112
#    3     [  6        1212           1     ]       112
#    4     [  1        1212           1     ]       112
#            ^-_ remove duplicates with worst times


# TREE #1:
# Feature:
#    kern_selected  Y
#    t_op           X
#    op_count       X
#    t_op_weight    X
#    t_total       ---cut


#NOTE: For tree 2, we need to do something about sets of unique X's producing
#           different t_totals.
#           IDEAS:  Random Drop
#                   Recency Drop
#                   Ranges (rolling min/max allowed)
#

# TREE #2:
#    t_total        Y   <-- bin this into ranges/stddev's?
#    kern_selected  X
#    t_op           X
#    op_count       X
#    t_op_weight    X


###
#
#  Some vector X that describes the kernels, Y is total
#
#  Things we might observe at runtime to use to produce the tree:
#     -- number of data elements
#     -- size of elements
#     -- thread count
#  
#  Row for each combination of [application] and [kernel]
#
#
###

    if (VERBOSE): print "== CONTROLLER:  Extracting dependent variable for Y-axis..."

    data["loop"] = pd.Categorical(data["loop"])
    data["loop_id"] = data["loop"].cat.codes

    if (VERBOSE): print "== CONTROLLER:  Exclude columns we don't want to use for learning..."
    #def pickMin(df):
    #    return df.loc(df["t_total"].astype(int).idxmin())
    #data.groupby(["policy_index", "group_id", "op_count", "t_total"]).filter(lambda x: )

    grp_data = data.sort_values("t_total").groupby(["policy_index", "op_count", "group_id"], as_index=False).first()
    y = grp_data["policy_index"].astype(int)
    x = grp_data.drop([
            "loop",
            "t_total",
            "policy_index",
            "group_id"
        ], axis="columns").values.astype(float)

    # This leaves X with...
    feature_names = ["frame", "group_id", "op_count", "t_op_weight", "t_op_avg"]

    if (VERBOSE): print "== CONTROLLER:  Initializing pipelines..."
    #####
    pipe = [('estimator',   DecisionTreeClassifier(
                 class_weight=None, criterion='gini', max_depth=2,
                 max_features=1, max_leaf_nodes=None,
                 min_impurity_split=1e-07, min_samples_leaf=1,
                 min_samples_split=2, min_weight_fraction_leaf=0.0,
                 presort=False, random_state=None, splitter='best'))]
    #####

    if (VERBOSE): print "== CONTROLLER:  Initializing model..."
    model = Pipeline(pipe)

    if (VERBOSE and (x.shape[0] > 10)):
        print "== CONTROLLER:  Cross-validation... (10-fold)"
        scores = cross_val_score(model, x, y, cv=min(10, (x.shape[0] - 1)))
        print("\n".join([("    " + str(score)) for score in scores]))
        print "    score.mean == " + str(np.mean(scores))

    if (VERBOSE): print "== CONTROLLER:  Training model..."
    model.fit(x, y)

    trained_model = model.named_steps['estimator']

    if (VERBOSE): print "== CONTROLLER:  Encoding rules..."
    #
    #
    rules_json = tree_to_json(trained_model, feature_names) 
    rules_code = tree_to_code(trained_model, feature_names)
    #rules_code = tree_to_string(trained_model, feature_names)

    dotfile = open("model.dot", 'w')
    from sklearn import tree as _tree
    _tree.export_graphviz(trained_model, out_file=dotfile, feature_names=feature_names)
    dotfile.close()

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['index'] = 4 
    model_def['type']['name'] = "DecisionTree"
    model_def['region_names'] = []
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



##########

def main():


    SOS.init()
    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    step = 0
    prior_frame_max = 0

    while (True):
        prior_frame_max = waitForMoreRows(sos_host, sos_port, prior_frame_max)

        # Submit the query to SOS, return results as a Pandas DataFrame:
        data, region_names = getTrainingData(sos_host, sos_port, row_limit=0);
    
        model_def = ""
        model_len = 0

        model_def, rules_code = generateDecisionTree(data, region_names)
        model_len = len(model_def)
        print "-----"
        print "STEP " + str(step) + " RULES:"
        print rules_code
        print "-----"
 
        model_def
        model_def= generateStaticModel(data, region_names)
        model_len = len(model_def)

        if model_len > 0:
            if (VERBOSE):
                print "== CONTROLLER:  Sending model to SOS runtime for distribution to Apollo..."
                print model_def
            SOS.trigger("APOLLO_MODELS", model_len, model_def)
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
            SOS.request_pub_manifest("", sos_host, sos_port)
    while (max_frame < (prior_frame_max + FRAME_INTERVAL)):
        print "== CONTROLLER:  Waiting for more data.  (" \
            + str(max_frame - prior_frame_max) + " of " \
            + str(FRAME_INTERVAL) + " new frames, " + str(max_frame) \
            + "  total. "
        time.sleep(1)
        max_frame, results, col_names = \
            SOS.request_pub_manifest("", sos_host, sos_port)
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
    # Eventually, we can drive an outer loop
    # that generates models for each unique
    # Apollo region in the workflow:
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
    region_names, col_names = SOS.query(sql_string, sos_host, sos_port) 
    if (VERBOSE):
        print "== CONTROLLER:  Loop name list received:"
        for row in region_names:
            print "    " + str(row[0])

    sql_string = """
        SELECT DISTINCT name FROM tblData;
    """
    if (VERBOSE):
        print "== CONTROLLER:  Retrieving possible field names."
    fields_avail, col_names = SOS.query(sql_string, sos_host, sos_port)
    if (VERBOSE):
        print "== CONTROLLER:  Field names available for query:"
        for row in fields_avail:
            print "    " + str(row[0])

    sql_string = """\
        SELECT frame, loop, policy_index, group_id, op_count, t_total, COALESCE(t_op_weight, 0) as t_op_weight, (t_total / op_count) AS t_op_avg FROM (
            SELECT
                  tblVals.frame AS frame,
                  tblData.guid AS guid,
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "loop"
                                  THEN tblVals.val END) AS "loop", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "policy_index"
                                  THEN tblVals.val END) AS "policy_index",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "group_id"
                                  THEN tblVals.val END) AS "group_id",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "op_count"
                                  THEN tblVals.val END) AS "op_count",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "t_total"
                                  THEN tblVals.val END) AS "t_total", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "t_op_weight"
                                  THEN tblVals.val END) AS "t_op_weight",
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
            loop"""

    if (row_limit < 1):
        sql_string += ";"
    else:
        sql_string += "LIMIT " + str(row_limit) + ";"

    results, col_names = SOS.query(sql_string, sos_host, sos_port)
    data = pd.DataFrame.from_records(results, columns=col_names)

    if (VERBOSE):
        print "== CONTROLLER:  Received training data from SOS..."
        tablePrint(results)

    return data, region_names


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
