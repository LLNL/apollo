#!/usr/bin/env python
import os
import sys
import json
import time
import datetime
import io
import warnings
import pytz
import dateutil

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

from apollo.utils import tablePrint



def generateDecisionTree(SOS, data, region_names):
    log(3, "Creating categorical values for unique regions.")
    data["region_name"] = pd.Categorical(data["region_name"])
    data["region_name_id"] = data["region_name"].cat.codes
    log(3, "== CONTROLLER:  Generating name-swapping table.")
    name_swap = data[["region_name", "region_name_id"]]\
            .groupby(["region_name_id"], as_index=False, sort=True)\
            .first()

    # NOTE: How to create a column of binned values for a column:
    #data["op_count_binned"] = pd.qcut(data["op_count"].astype(float), 50, duplicates="drop")

    # NOTE: We use time_avg instead of time_min/max/last because there will be
    #       different numbers of operations as we sweep through different
    #       dimensions/configs. We don't want to be picking some kernel
    #       just because it got a super low time doing only 1 operation.
    #       We want best on average.
    log(3, "Sorting, grouping, and pruning data.shape(" + str(data.shape) + ")")

    # OLD technique:
    #start = time.time()
    #grp_data = data\
    #        .sort_values("time_avg")\
    #        .groupby(["region_name", "num_elements", "policy_index"], as_index=False, sort=False)\
    #        .first()\
    #        .dropna()
    #elapsed_chain = time.time() - start

    start = time.time()
    grp_data = data.groupby(
            ["region_name", "region_name_id", "num_elements", "policy_index"],
            as_index=False).agg({'time_avg':min})
    elapsed_agg = time.time() - start

    log(3, "data.groupby.agg() took " + str(elapsed_agg) + " seconds.")

    # if (VERBOSE >= 9):
    #    log(9, "Grouped data:")
    #    tablePrint(grp_data.astype(str).values.tolist())
    #    log(9, "    " + str(grp_data.columns.astype(str).values.tolist()))

    # Get the translation table to go back and forth from categorical to actual
    # region name. We'll need this after doing the model.fit() step
    if (VERBOSE >= 9):
        log(9, "Name swapping table:")
        tablePrint(name_swap[["region_name", "region_name_id"]].astype(str).values.tolist())

    drop_fields = ["region_name", "region_name_id", "policy_index", "time_avg"]

    feature_names = [f for f in grp_data.columns if f not in drop_fields]
    log(9, "Feature names: " + str(feature_names))
    log(9, "Creating a vector for regional data and models ...")

    model_count = 0
    all_types_rule = {}
    all_rules_json = {}
    all_least_json = {}
    all_timed_json = {}
    all_sizes_data = {}
    overall_start = time.time()

    for region in region_names:
        model_count += 1
        this_start = time.time()

        rd = grp_data[grp_data['region_name'] == region]

        if (rd.shape[0] < 1):
            continue

        element_minimum_to_evaluate_tree = -1
        # seq_winners =  rd[rd['policy_index']==1]
        # if (seq_winners.shape[0] > 0):
        #     element_minimum_to_evaluate_tree = seq_winners.max('num_elements').astype(int)

        y = rd["policy_index"].astype(int)
        x = rd.drop(drop_fields, axis="columns").values.astype(float)

        # NOTE: Prior hyper-specific method...
        #pipe = [('estimator',   DecisionTreeClassifier(
        #         class_weight=None, criterion='gini', max_depth=6,
        #         max_features=x.shape[1], max_leaf_nodes=None,
        #         min_impurity_decrease=1e-07, min_samples_leaf=1,
        #         min_samples_split=2, min_weight_fraction_leaf=0.0,
        #         presort=False, random_state=None, splitter='best'))]
        pipe = [('estimator',   DecisionTreeClassifier(
                 class_weight=None, criterion='gini', max_depth=4,
                 min_samples_leaf=1, min_samples_split=2))]
        model = Pipeline(pipe)

        model.fit(x, y)

        trained_model = model.named_steps['estimator']

        all_types_rule[region] = "DecisionTree"
        all_rules_json[region] = tree_to_data(trained_model, feature_names, name_swap, y)
        all_least_json[region] = element_minimum_to_evaluate_tree
        all_timed_json[region] = True
        all_sizes_data[region] = str(x.shape)

        this_elapsed = time.time() - this_start

        log(3, "Built a model for region #" + str(model_count) + " \"" + str(region) \
                + "\" having x.shape == " + str(x.shape) + " in " + str(this_elapsed) \
                + " seconds.")

        #dotfile = open("model.dot", 'w')
        #from sklearn import tree as _tree
        #_tree.export_graphviz(trained_model, out_file=dotfile, feature_names=feature_names)
        #dotfile.close()

    overall_elapsed = time.time() - overall_start
    log(2, "OK: Fit" + str(model_count) + " models in " + str(overall_elapsed) + " seconds.")

    json_start = time.time()

    model_def = {
        "guid": SOS.get_guid(),
        "driver": {
            "rules": all_rules_json,
            "least": all_least_json,
            "timed": all_timed_json,
        },
        "region_names": list(region_names),
        "region_sizes": all_sizes_data,
        "region_types": all_types_rule,
        "features": {
            "count": len(feature_names),
            "names": feature_names,
        },
    }


    #IDEA: Create bins of regions. If some region doesn't have a model, figure out what bin
    #      its actual runtimes placed it in and give it a model from some other region in that
    #      bin... (this is problematic, just an idea)

    # Add in a default model (Static, OMP defaults) for any unnamed region:
    model_def["region_names"].append("__ANY_REGION__")
    model_def["region_sizes"]["__ANY_REGION__"] = "(0, 0)"
    model_def["region_types"]["__ANY_REGION__"] = "Static"
    model_def["driver"]["rules"]["__ANY_REGION__"] = "0"
    model_def["driver"]["least"]["__ANY_REGION__"] = -1
    model_def["driver"]["timed"]["__ANY_REGION__"] = True

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4, ensure_ascii=True) + "\n"
    json_elapsed = time.time() - json_start
    log(3, "Serializing models into JSON took " + str(json_elapsed) + " seconds.")

    return model_as_json

    #  Keeping this here for an example.
    #  Cross-Validation doesn't work well with the simplified/conditioned data.
    #if (VERBOSE > 3 and x.shape[0] > 2):
    #    warnings.filterwarnings("ignore")
    #    cv_folds = 10
    #    log(3, "Cross-validation... (" + str(cv_folds) + "-fold)")
    #    scores = cross_val_score(model, x, y, cv=cv_folds)
    #    log(3, "\n".join([("    " + str(score)) for score in scores]))
    #    log(3, "    score.mean == " + str(np.mean(scores)))
    #    warnings.resetwarnings()




def tree_to_data(decision_tree, feature_names=None, name_swap=None, y=None):
    def node_to_data(tree, node_id, criterion):
        if not isinstance(criterion, skl.tree.tree.six.string_types):
            criterion = "impurity"

        value = tree.value[node_id]
        if tree.n_outputs == 1:
            value = value[0, :]

        if tree.children_left[node_id] == skl.tree._tree.TREE_LEAF:
            # Construct a padded value based on index position:
            padval = [0.0] * 20
            for i in range(0, (len(value) - 1)):
                policy_positions = list(y)
                policy_index = policy_positions[i]
                padval[policy_index] += value[i]
            return {
                "id": node_id,
                "criterion": criterion,
                "impurity": tree.impurity[node_id],
                "samples": tree.n_node_samples[node_id],
                "value": padval,
            }
            #return {
            #    "id": node_id,
            #    "criterion": criterion,
            #    "impurity": tree.impurity[node_id],
            #    "samples": tree.n_node_samples[node_id],
            #    "value": list(value),
            #}
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

            return {
                "id": node_id,
                "rule": "%s %s %s" % (feature, ruleType, ruleValue),
                criterion: tree.impurity[node_id],
                "samples": tree.n_node_samples[node_id],
            }

    def recurse(tree, node_id, criterion, parent=None, depth=0):
        left_child = tree.children_left[node_id]
        right_child = tree.children_right[node_id]

        node = node_to_data(tree, node_id, criterion)

        if left_child != skl.tree._tree.TREE_LEAF:
            node["left"] = recurse(tree,
                                   left_child,
                                   criterion=criterion,
                                   parent=node_id,
                                   depth=depth + 1)
            node["right"] = recurse(tree,
                                    right_child,
                                    criterion=criterion,
                                    parent=node_id,
                                    depth=depth + 1)

        return node

    if isinstance(decision_tree, skl.tree.tree.Tree):
        return recurse(decision_tree, 0, criterion="impurity")
    else:
        return recurse(decision_tree.tree_, 0, criterion=decision_tree.criterion)




##########################




def generateRegressionTree(SOS, data, region_names):
    # Make a numeric representation of loop name strings:
    data["loop"] = pd.Categorical(data["loop"])
    data["loop_id"] = data["loop"].cat.codes

    # TODO: This is all out of date...

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

    log(9, str(pol_stds))
    quit()

    feature_names = []
    raw_names = data.drop(drop_fields, axis="columns").columns
    for name in raw_names:
        feature_names.append(name)

    predictedTime = DecisionTreeRegressor()
    predictedTime.fit(x, y)

    #leafStdDev = np.std(y)

    log(9, "predictedTime = " + str(predictedTime))
    log(9, "predictions table:")
    comp = ""
    for row in x:
        for column in row:
            comp += ("[" + str(int(column)) + "]")
        comp += (" == " + str(predictedTime.predict(row.reshape(1, -1))))
        #comp += (" @ " + str(leafStdDev))
        #comp += (" @ " + str(pol_stds[row["policy_index"]))
        comp += "\n"
    log(9, comp)

    #with open("regress.dot", 'w') as dotfile:
    #    from sklearn import tree as _tree
    #    _tree.export_graphviz(predictedTime, out_file=dotfile, feature_names=feature_names)

    #reg_tree = json.dumps(serializeRegressor(predictedTime))
    #with open("regress.json", 'w') as regfile:
    #    regfile.write(reg_tree)

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



