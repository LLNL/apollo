#!/usr/bin/env python
import os
import sys
import json
import time
import cStringIO
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

VERBOSE = True
FRAME_INTERVAL = 1000 
SOS = SSOS() 

def serializeRegressor(tree):
    """ Convert a sklearn.tree.DecisionTreeRegressor into a JSON-compatible format """
    LEAF_ATTRIBUTES = ['children_left', 'children_right', 'threshold', 'value',
                       'feature', 'impurity', 'weighted_n_node_samples']
    TREE_ATTRIBUTES = ['n_classes_', 'n_features_', 'n_outputs_']
    encoded = {
        'nodes': {},
        'tree': {},
        'n_leaves': len(tree.tree_.threshold),
        'params': tree.get_params()
    }
    for attr in LEAF_ATTRIBUTES:
        encoded['nodes'][attr] = getattr(tree.tree_, attr).tolist()
    for attr in TREE_ATTRIBUTES:
        encoded['tree'][attr] = getattr(tree, attr)
    return encoded


def generateRegressionTree(data, region_names):
    # Make a numeric representation of loop name strings:
    data["loop"] = pd.Categorical(data["loop"])
    data["loop_id"] = data["loop"].cat.codes

    # We don't bin or simplify the raw data for regression trees,
    # we want all of it.
    drop_fields =[
            "frame",
            "loop",
            "loop_id",
            "t_op_avg",
            "t_total"
        ] 
    
    y = data["t_total"].astype(float)
    x = data.drop(drop_fields, axis="columns").values.astype(float)

    feature_names = []
    raw_names = data.drop(drop_fields, axis="columns").columns
    for name in raw_names:
        feature_names.append(name)

    predictedTime = DecisionTreeRegressor()
    predictedTime.fit(x, y)

    print "score: " + str()

    print str(predictedTime)

    print "prediction using: " + str(feature_names)
    print "    " + str(x[-1]) + "    == " + str(predictedTime.predict(x[-1].reshape(1, -1)))

    dotfile = open("regress.dot", 'w')
    from sklearn import tree as _tree
    _tree.export_graphviz(predictedTime, out_file=dotfile, feature_names=feature_names)
    dotfile.close()

    reg_tree = json.dumps(serializeRegressor(predictedTime))
    regfile = open("regress.json", 'w')
    regfile.write(reg_tree)
    regfile.close()
    return



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

    # NOTE: For tree 2, we need to do something about sets of unique X's producing
    #           different t_totals.
    #           IDEAS:  Random Drop
    #                   Recency Drop

    # CONCEPT #2
    #   Swap in the new model for n% of iterations and see if it is better
    #      (some form of A/B testing)

    # CONCEPT #3 (TREE 2)
    #    t_total        Y   <-- bin this into ranges/stddev's?
    #    kern_selected  X
    #    t_op           X
    #    op_count       X
    #    op_weight      X

    # CONCEPT #4
    #     Show that features are evolving over time, "zoom in on the ones that matter"
    #     This means keeping the old data around.
    #   -- show different architectures
    #   -- vectorized vs. non-vectorized (RAJA kernel variants?)
    #   -- show debug vs. optimized build options

    
    data["loop"] = pd.Categorical(data["loop"])
    data["loop_id"] = data["loop"].cat.codes

    # Example of value binning (up to 50 bins, dropping any bins with duplicated edge values):
    #data["op_count_binned"] = pd.qcut(data["op_count"].astype(float), 50, duplicates="drop")
    #grp_data = data\
    #        .sort_values("t_op_avg")\
    #        .groupby(["op_count_binned"], as_index=False)\
    #        .first()

    # NOTE: We use t_op_avg instead of t_total because there will be different numbers of operations
    #       as we sweep through different dimensions/configs. We don't want to be picking some kernel
    #       just because it got a super low time doing only 1 operation.  We want best on average.
    grp_data = data\
            .sort_values("t_op_avg")\
            .groupby(["vector_size"], as_index=False, sort=False)\
            .first()


    drop_fields =[
            "frame",
            "loop",
            "loop_id",
            "policy_index",
            "t_op_avg",
            "t_total"
        ] 
    
    y = grp_data["policy_index"].astype(int)
    x = grp_data.drop(drop_fields, axis="columns").values.astype(float)

    feature_names = []
    raw_names = grp_data.drop(drop_fields, axis="columns").columns
    for name in raw_names:
        feature_names.append(name)
 
    
    if (VERBOSE): print "== CONTROLLER:  Initializing model..."
    #pipe = [('estimator', DecisionTreeClassifier())]
    pipe = [('estimator',   DecisionTreeClassifier(
                 class_weight=None, criterion='gini', max_depth=7,
                 max_features=len(feature_names), max_leaf_nodes=None,
                 min_impurity_split=1e-07, min_samples_leaf=1,
                 min_samples_split=2, min_weight_fraction_leaf=0.0,
                 presort=False, random_state=None, splitter='best'))]
    model = Pipeline(pipe)


    # NOTE: Cross-Validation doesn't work well with the simplified/conditioned data.
    #if (VERBOSE and x.shape[0] > 2):
    #    warnings.filterwarnings("ignore")
    #    cv_folds = 10 
    #    print "== CONTROLLER:  Cross-validation... (" + str(cv_folds) + "-fold)"
    #    scores = cross_val_score(model, x, y, cv=cv_folds)
    #    print("\n".join([("    " + str(score)) for score in scores]))
    #    print "    score.mean == " + str(np.mean(scores))
    #    warnings.resetwarnings()

    if (VERBOSE): print "== CONTROLLER:  Training model..."
    model.fit(x, y)

    trained_model = model.named_steps['estimator']

    if (VERBOSE): print "== CONTROLLER:  Encoding rules..."
    rules_json = tree_to_json(trained_model, feature_names) + "\n"
    rules_code = tree_to_code(trained_model, feature_names)


    dotfile = open("model.dot", 'w')
    from sklearn import tree as _tree
    _tree.export_graphviz(trained_model, out_file=dotfile, feature_names=feature_names)
    dotfile.close()

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['guid'] = SOS.get_guid() 
    model_def['type']['name'] = "DecisionTree"
    model_def['region_names'] = []
    for n in region_names:
        for nm in n:
            model_def['region_names'].append(nm)
    model_def['features'] = {}
    model_def['features']['count'] = len(feature_names)
    model_def['features']['names'] = []
    model_def['features']['names'] = feature_names
    model_def['driver'] = {}
    model_def['driver']['format'] = "json"
    model_def['driver']['rules'] = rules_json #json.dumps(trained_model, sort_keys=False, indent=4)  

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4, ensure_ascii=True) + "\n"

    return model_as_json, rules_code



##########

def main():


    SOS.init()
    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    step = 0
    prior_frame_max = 0

    while (True):
        prior_frame_max    = waitForMoreRows(sos_host, sos_port, prior_frame_max)
        data, region_names = getTrainingData(sos_host, sos_port, row_limit=0);
   
        model_def = ""
        model_len = 0

        # DECISIONTREE
        #model_def, rules_code = generateDecisionTree(data, region_names)
        #model_len = len(model_def)
        #print rules_code

        # REGRESSIONTREE
        generateRegressionTree(data, region_names)
        quit()

        # STATIC
        #model_def = generateStaticModel(data, region_names)
        #model_len = len(model_def)

        if model_len > 0:
            if (VERBOSE):
                print "== CONTROLLER:  Sending >>> DECISIONTREE <<< to SOS for Apollo..."
                print "== CONTROLLER:    ..."
            SOS.trigger("APOLLO_MODELS", model_len, model_def)
            if (VERBOSE): print "== CONTROLLER:  Pausing to allow new model to run for a fresh interval ..."
            waitForMoreRows(sos_host, sos_port, prior_frame_max);

            if (VERBOSE): print "== CONTROLLER:  Clearing prior training data..."
            wipeTrainingData(sos_host, sos_port, prior_frame_max)
            
            if (VERBOSE):
                print "== CONTROLLER:  Sending >>> RANDOMSEARCH <<< to SOS for Apollo..."
                print "== CONTROLLER:    ..."
            model_def = generateRandomModel(data, region_names)
            model_len = len(model_def)
            SOS.trigger("APOLLO_MODELS", model_len, model_def)
            # Reset the frame max to so we give the controller new random stuff to learn on
            # TODO: We need to ignore data from the previous optimal run...
            prior_frame_max, pub_titles, col_names = \
                SOS.request_pub_manifest("", sos_host, sos_port)
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
        sys.stdout.write("== CONTROLLER:  Waiting for data. " \
            + "[" + progressBar((max_frame - prior_frame_max), FRAME_INTERVAL, 20) + "] " \
            + "( " + str(max_frame - prior_frame_max) + " of " \
            + str(FRAME_INTERVAL) + ", " + str(max_frame) \
            + " total)\r")
        sys.stdout.flush()
        time.sleep(1)
        max_frame, results, col_names = \
            SOS.request_pub_manifest("", sos_host, sos_port)

    #####
    sys.stdout.write("== CONTROLLER:  Waiting for data. " \
        + "[" + progressBar((max_frame - prior_frame_max), FRAME_INTERVAL, 20) + "] " \
        + "( " + str(max_frame - prior_frame_max) + " of " \
        + str(FRAME_INTERVAL) + ", " + str(max_frame) \
        + " total)\n")
    sys.stdout.flush()
    return max_frame

def tablePrint(results):
    # Print out the results in a pretty column-aligned way:
    widths = [max(map(len, str(col))) for col in zip(*results)]
    for row in results: 
        print "  ".join((val.ljust(width) for val, width in zip(row, widths)))
    #
    return


def wipeTrainingData(sos_host, sos_port, prior_frame_max):
    sql_string =  "DELETE FROM tblVals "
    sql_string += "WHERE tblVals.frame < " + str(prior_frame_max) + ";"
    region_names, col_names = SOS.query(sql_string, sos_host, sos_port) 
    return


def getTrainingData(sos_host, sos_port, row_limit):
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

    sql_string = """\
        SELECT
            frame,
            loop,
            vector_size,
            policy_index,
            sum_time_inclusive_duration AS t_total,
            ((sum_time_inclusive_duration * 1.0) / (vector_size * 1.0)) AS t_op_avg
        FROM (
            SELECT
                  tblVals.frame AS frame,
                  tblData.guid AS guid,
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "loop"
                                  THEN tblVals.val END) AS "loop", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "policy_index"
                                  THEN tblVals.val END) AS "policy_index",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "vector_size"
                                  THEN tblVals.val END) AS "vector_size",
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

    results, col_names = SOS.query(sql_string, sos_host, sos_port)
    data = pd.DataFrame.from_records(results, columns=col_names)

    if (VERBOSE):
        print "== CONTROLLER:  Received training data from SOS..."
        tablePrint(results)

    return data, region_names


def generateRandomModel(data, region_names):

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['guid'] = SOS.get_guid() 
    model_def['type']['name'] = "Random"
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
    
    model_as_json = json.dumps(model_def, sort_keys=False, indent=4,
            ensure_ascii=True)
    return model_as_json

def generateStaticModel(data, region_names):

    model_def = {} 
    model_def['type'] = {}
    model_def['type']['guid'] = SOS.get_guid()
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
    
    model_as_json = json.dumps(model_def, sort_keys=False, indent=4,
            ensure_ascii=True)
    return model_as_json

#########

from sklearn.tree import _tree



def progressBar(amount, total, length, fill='='):
    if amount >= total:
        return fill * length
    if length < 4: length = 4
    fillLen = int(length * amount // total)
    emptyLen = length - 1 - fillLen
    bar = (fill * fillLen) + ">" + ("-" * emptyLen)
    return bar


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

def json_load_byteified(file_handle):
    return _byteify(
        json.load(file_handle, object_hook=_byteify),
        ignore_dicts=True
    )

def json_loads_byteified(json_text):
    return _byteify(
        json.loads(json_text, object_hook=_byteify),
        ignore_dicts=True
    )

def _byteify(data, ignore_dicts = False):
    # if this is a unicode string, return its string representation
    if isinstance(data, unicode):
        return data.encode('utf-8')
    # if this is a list of values, return list of byteified values
    if isinstance(data, list):
        return [ _byteify(item, ignore_dicts=True) for item in data ]
    # if this is a dictionary, return dictionary of byteified keys and values
    # but only if we haven't already byteified it
    if isinstance(data, dict) and not ignore_dicts:
        return dict((_byteify(key,
            ignore_dicts=True),
            _byteify(value, ignore_dicts=True)) \
                for key, value in data.iteritems())
    # if it's anything else, return it in its original form
    return data


if __name__ == "__main__":
    main()
