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

sos = SSOS() 

# TODO: Make Python class with unimplemented members:
#           -time between polling
#           -...etc.  The tunable knobs we want to search
#            over in the paper.
#           

def main():

    sos.init()

    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    # INSERT HERE: Loop to check that "enough" data has come in.

    # Submit the query to SOS, return results as a DataFrame:
    data, loop_names = getTrainingData(sos_host, sos_port, row_limit=0);
    
    # Remove the redundant stuff from Caliper:
    data = data[data.event_end_loop != "NULL"]

    # Process an example ML pipeline w/hardcoded field names:
    model_def = ""
    model_len = 0

    try:
        model_def = learningExample(data, loop_names)
        model_len = len(model_def)
    except ValueError:
        print ">>>>> EXCEPTION: (ValueError)"
        print "      Unable to learn using training Pandas dataFrame content."
        print "      This is likely due a to the format of query results."

    if model_len > 0:
        print ">>>>> Sending model to SOS runtime for distribution to Apollo..."
        print "      ----------"
        print model_def
        print "      ----------"
        sos.trigger("APOLLO_MODELS", model_len, model_def)
    else:
        print ">>>>> NOTICE: Model was not generated, nothing to send."

    print ">>>>> Done."
    return

#########


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

    print ">>>>> Retrieving list of unique loops being measured."
    loop_names, col_names = sos.query(sql_string, sos_host, sos_port) 
    print ">>>>> Loop name list received:"
    tablePrint(loop_names)
    

    sql_string = """
        SELECT * FROM (
            SELECT
                  tblVals.frame AS frame,
                  tblData.guid AS guid,
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.attr.level"
                                  THEN tblVals.val END) AS "cali_event_attr_level", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "vector_size"
                                  THEN tblVals.val END) AS "vector_size", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.end"
                                  THEN tblVals.val END) AS "cali_event_end", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "loop"
                                  THEN tblVals.val END) AS "loop", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "policyIndex"
                                  THEN tblVals.val END) AS "policyIndex", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "sum#time.inclusive.duration"
                                  THEN tblVals.val END) AS "sum_time_inclusive_duration", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "event.end#loop"
                                  THEN tblVals.val END) AS "event_end_loop" 
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

    print ">>>>> Sending a query for training data..."
    #print "      ----------"
    #print sql_string.translate(None, '\n')
    #print "      ----------"
    results, col_names = sos.query(sql_string, sos_host, sos_port)
    data = pd.DataFrame.from_records(results, columns=col_names)
  


    print ">>>>> Received training data from SOS..."

    return data, loop_names


def learningExample(data, loop_names): 
    #print "numpy.__version__   == " + str(np.__version__)
    #print "pandas.__version__  == " + str(pd.__version__)
    #print "sklearn.__version__ == " + str(skl.__version__)

    # Available fields:
    #     frame
    #     guid
    #     cali_event_attr_level
    #     vector_size
    #     cali_event_end
    #     loop
    #     policyIndex
    #     sum_time_inclusive_duration
    #     event_end_loop

    print ">>>>> Extracting dependent variable for Y-axis..."
    # Demo uses the Kripke data:
    data = pd.read_csv("./src/python/kripke_sample.csv", index_col=False)
    y = (data["percent_of_fastest"].values.astype(float)/10).astype(int)
    x = data.drop("percent_of_fastest", axis="columns").values.astype(float)

    # NOTE: Here binned into every 10% for DecisionTreeClassifier 
    #y = (data["sum_time_inclusive_duration"].values.astype(float)/10).astype(int)
    #y = y.squeeze()
    # Exclude columns we don't want to use for learning:
    #x = data.drop([
    #        "frame",
    #        "guid",
    #        "cali_event_attr_level",
    #        #"vector_size",
    #        "cali_event_end",
    #        "loop",
    #        #"policyIndex",
    #        "sum_time_inclusive_duration",
    #        "event_end_loop"
    #    ], axis="columns").values

    print ">>>>> Initializing pipelines..."
    basic_pipe = [('estimator',   DecisionTreeClassifier(max_depth=None))]
    dtree_pipe = [('standardize', StandardScaler()),\
                  ('estimator',   DecisionTreeClassifier(
                      class_weight=None, criterion='gini', max_depth=5,
                      max_features=5, max_leaf_nodes=None,
                      min_impurity_split=1e-07, min_samples_leaf=1,
                      min_samples_split=2, min_weight_fraction_leaf=0.0,
                      presort=False, random_state=None, splitter='best'))]
    rtree_pipe = [('estimator',   DecisionTreeRegressor(max_depth=5))]
    svc_pipe =   [('standardize', StandardScaler()),\
                 ('estimator',    SVC())]

    print ">>>>> Selecting pipe..."
    #pipe = basic_pipe
    #pipe = rtree_pipe
    pipe = dtree_pipe
    #pipe = svc_pipe

    print ">>>>> Initializing model..."
    model = Pipeline(pipe)

    #print ">>>>> Cross-validation... (10-fold)"
    #scores = cross_val_score(model, x, y, cv=10)
    #print("\n".join([("    " + str(score)) for score in scores]))
    #print "    score.mean == " + str(np.mean(scores))

    print ">>>>> Training model..."
    model.fit(x, y)

    trained_model = model.named_steps['estimator']
    feature_names = ["kernel","ltimes", "lplustimes","population", "scattering", "source", "sweep"] 

    print ">>>>> Encoding rules..."  #See below, also:   tree_to_code(...)
    #rules = tree_to_json(trained_model)
    #rules = tree_to_string(trained_model, ["vector_size", "policyIndex"])
    #rules = tree_to_string(trained_model, ["kernel","ltimes", "lplustimes","population", "scattering", "source", "sweep"])
    rules = tree_to_json(trained_model, feature_names)

    print "-----"
    print rules
    print "-----"

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['index'] = 3
    model_def['type']['name'] = "DecisionTree"
    model_def['loop_names'] = loop_names
    model_def['features'] = {}
    model_def['features']['count'] = len(feature_names)
    model_def['features']['names'] = []
    model_def['features']['names'] = feature_names
    model_def['driver'] = {}
    model_def['driver']['format'] = "json"
    model_def['driver']['rules'] = rules 

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4)

    return model_as_json



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
        for i in tree_.feature
    ]

  
    #Begin recursively encoding the decision tree:
    def recurseSTR(result_str, node, depth):
        # NOTE: Useful for debugging trees:
        #       offset = "  " * depth
        #
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
    feature_name = [
        feature_names[i] if i != _tree.TREE_UNDEFINED else "undefined!"
        for i in tree_.feature
    ]
    print "def tree({}):".format(", ".join(feature_names))

    def recurse(node, depth):
        indent = "  " * depth
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            print "{}if {} <= {}:".format(indent, name, threshold)
            recurse(tree_.children_left[node], depth + 1)
            print "{}else:  # if {} > {}".format(indent, name, threshold)
            recurse(tree_.children_right[node], depth + 1)
        else:
            print "{}return {}".format(indent, tree_.value[node])

    recurse(0, 1)


if __name__ == "__main__":
    main()
