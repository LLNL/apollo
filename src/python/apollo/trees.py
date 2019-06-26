#!/usr/bin/env python
import os
import sys
import json
import time
import datetime
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

from config import VERBOSE
from config import DEBUG
from config import FRAME_INTERVAL

import utils





def generateDecisionTree(SOS, data, region_names):
    if (VERBOSE): print "== CONTROLLER:  Creating categorical values for unique regions."
    data["region_name"] = pd.Categorical(data["region_name"])
    data["region_name_id"] = data["region_name"].cat.codes

    # NOTE: How to create a column of binned values for a column:
    #data["op_count_binned"] = pd.qcut(data["op_count"].astype(float), 50, duplicates="drop")

    # NOTE: We use time_avg instead of time_min/max/last because there will be
    #       different numbers of operations as we sweep through different
    #       dimensions/configs. We don't want to be picking some kernel
    #       just because it got a super low time doing only 1 operation.
    #       We want best on average.
    if (VERBOSE): print "== CONTROLLER:  Sorting, grouping, and pruning data."
    data.set_index(['region_name'])

    grp_data = data\
            .sort_values("time_avg")\
            .groupby(["region_name", "num_elements"], as_index=False, sort=False)\
            .first()\
            .dropna()

    grp_data.set_index(['region_name'])

    #if (DEBUG):
    #    print "== [[ DEBUG ]]  Grouped data:"
    #    utils.tablePrint(grp_data.astype(str).values.tolist())
    #    print "    " + str(grp_data.columns.astype(str).values.tolist())

    # Get the translation table to go back and forth from categorical to actual
    # region name. We'll need this after doing the model.fit() step
    if (VERBOSE): print "== CONTROLLER:  Generating name-swapping table."
    name_swap = data[["region_name", "region_name_id"]]\
            .groupby(["region_name_id"], as_index=False, sort=True)\
            .first()\
            .dropna()
    #if (DEBUG):
    #    print "== [[ DEBUG ]]  Name swapping table:"
    #    utils.tablePrint(name_swap[["region_name", "region_name_id"]].astype(str).values.tolist())


    # Available fields:
    #        region_name,
    #        region_name_id,
    #        policy_index,
    #        step,
    #        frame,
    #        num_elements,
    #        exec_count,
    #        time_last,
    #        time_min,
    #        time_max,
    #        time_avg
    drop_fields = [
            "region_name",
            "region_name_id",
            "policy_index",
            "mpi_rank",
            "step",
            "exec_count",
            "frame",
            "time_last",
            "time_min",
            "time_max",
            "time_avg" ]

    feature_names = []
    raw_names = grp_data.drop(drop_fields, axis="columns").columns
    for name in raw_names:
        feature_names.append(name)
    if (VERBOSE):
	print "== CONTROLLER:  Feature names: " + str(feature_names)

    # TODO: Set up loop and start making models for each region.
    print "== CONTROLLER:  Creating a vector for regional data and models ..."
    all_rules_json = {}
    all_rules_code = {}
    for region in region_names:
        all_rules_json[region] = ""
        all_rules_code[region] = ""
        rd = grp_data[grp_data['region_name'] == region]
        if (VERBOSE): print "== CONTROLLER:  Building a model for: " + str(region)
        y = rd["policy_index"].astype(int)
        x = rd.drop(drop_fields, axis="columns").values.astype(float)
        #print str(feature_names)
        #utils.tablePrint(x.astype(str))


        if (VERBOSE): print "== CONTROLLER:  Initializing model..."
        pipe = [('estimator',   DecisionTreeClassifier(
                 class_weight=None, criterion='gini', max_depth=3,
                 max_features=len(feature_names), max_leaf_nodes=None,
                 min_impurity_decrease=1e-07, min_samples_leaf=1,
                 min_samples_split=2, min_weight_fraction_leaf=0.0,
                 presort=False, random_state=None, splitter='best'))]
        model = Pipeline(pipe)

        if (VERBOSE):
            print "== CONTROLLER:  Running model.fit(x" + str(x.shape) + ", y" + str(y.shape) + ")"

        model.fit(x, y)

        if (VERBOSE):
            #time_training = datetime.now() - time_start
            time_training = -1

        trained_model = model.named_steps['estimator']

        if (VERBOSE): print "== CONTROLLER:  Encoding rules..."

        all_rules_json[region] = tree_to_json(trained_model, feature_names, name_swap) + "\n"
        all_rules_code[region] = tree_to_code(trained_model, feature_names)

        #dotfile = open("model.dot", 'w')
        #from sklearn import tree as _tree
        #_tree.export_graphviz(trained_model, out_file=dotfile, feature_names=feature_names)
        #dotfile.close()

    model_def = {}
    model_def['type'] = {}
    model_def['type']['guid'] = SOS.get_guid()
    model_def['type']['name'] = "DecisionTree"
    model_def['region_names'] = []
    for region in region_names:
        model_def['region_names'].append(region)
    model_def['features'] = {}
    model_def['features']['count'] = len(feature_names)
    model_def['features']['names'] = []
    model_def['features']['names'] = feature_names
    model_def['driver'] = {}
    model_def['driver']['format'] = "json"
    model_def['driver']['rules'] = {}
    for region in region_names:
        model_def['driver']['rules'][str(region)] = all_rules_json[region]

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4, ensure_ascii=True) + "\n"

    return model_as_json, all_rules_code


    #  Keeping this here for an example.
    #  Cross-Validation doesn't work well with the simplified/conditioned data.
    #if (VERBOSE and x.shape[0] > 2):
    #    warnings.filterwarnings("ignore")
    #    cv_folds = 10
    #    print "== CONTROLLER:  Cross-validation... (" + str(cv_folds) + "-fold)"
    #    scores = cross_val_score(model, x, y, cv=cv_folds)
    #    print("\n".join([("    " + str(score)) for score in scores]))
    #    print "    score.mean == " + str(np.mean(scores))
    #    warnings.resetwarnings()



def generateRegressionTree(SOS, data, region_names):
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

    pol_stds = data.groupby(["policy_index"], as_index=False)\
        [["t_total"]].apply(np.std)

    print str(pol_stds)
    quit()

    feature_names = []
    raw_names = data.drop(drop_fields, axis="columns").columns
    for name in raw_names:
        feature_names.append(name)

    predictedTime = DecisionTreeRegressor()
    predictedTime.fit(x, y)

    #leafStdDev = np.std(y)

    print str(predictedTime)
    print "predictions table:"
    comp = ""
    for row in x:
        for column in row:
            comp += ("[" + str(int(column)) + "]")
        comp += (" == " + str(predictedTime.predict(row.reshape(1, -1))))
        #comp += (" @ " + str(leafStdDev))
        #comp += (" @ " + str(pol_stds[row["policy_index"]))
        comp += "\n"
    print comp

    dotfile = open("regress.dot", 'w')
    from sklearn import tree as _tree
    _tree.export_graphviz(predictedTime, out_file=dotfile, feature_names=feature_names)
    dotfile.close()

    reg_tree = json.dumps(serializeRegressor(predictedTime))
    regfile = open("regress.json", 'w')
    regfile.write(reg_tree)
    regfile.close()
    return reg_tree


# def serializeRegressor(tree):
#     """ Convert a sklearn.tree.DecisionTreeRegressor into a JSON-compatible format """
#     LEAF_ATTRIBUTES = ['children_left', 'children_right', 'threshold', 'value',
#                        'feature', 'impurity', 'weighted_n_node_samples']
#     TREE_ATTRIBUTES = ['n_classes_', 'n_features_', 'n_outputs_']
#     encoded = {
#         'nodes': {},
#         'tree': {},
#         'n_leaves': len(tree.tree_.threshold),
#         'params': tree.get_params()
#     }
#     for attr in LEAF_ATTRIBUTES:
#         encoded['nodes'][attr] = getattr(tree.tree_, attr).tolist()
#     for attr in TREE_ATTRIBUTES:
#         encoded['tree'][attr] = getattr(tree, attr)
#     return encoded



from sklearn.tree import _tree
def tree_to_json(decision_tree, feature_names=None, name_swap=None):
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

def tree_to_string(tree, feature_names=None, name_swap=None):
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


