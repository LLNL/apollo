#!/usr/bin/env python3

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
import csv
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
from sklearn                 import metrics
from sklearn.preprocessing   import StandardScaler
from sklearn.tree            import DecisionTreeClassifier
from sklearn.tree            import DecisionTreeRegressor
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.model_selection import train_test_split
from sklearn.svm             import SVC

from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT

from apollo.utils import tablePrint


def generateRegressionTree(log, data,
        assign_guid=0,
        tree_max_depth=2,
        one_big_tree=False):

    region_name_dict = {}
    for row in data.itertuples():
        key = (row.region_name)
        if key not in region_name_dict:
            region_name_dict[key] = str(row.region_name)
    region_names = []
    for entry in region_name_dict.items():
        key, value = entry
        region_names.append(value)

    log(3, "Creating categorical values for unique regions.")
    data["region_name"] = pd.Categorical(data["region_name"])
    data["region_name_id"] = data["region_name"].cat.codes
    log(3, "== CONTROLLER:  Generating name-swapping table.")
    name_swap = data[["region_name", "region_name_id"]]\
            .groupby(["region_name_id"], as_index=False, sort=True)\
            .first()

    log(3, "Sorting, grouping, and pruning data.shape(" + str(data.shape) + ")")
    grp_data = data # Grouping is done in SQL for now.

    # Drop "step" because that's going to be asymptotically increasing as
    # the simulation proceeds, not useful for making time predictions of
    # future data, but might come in handy if we start to re-use models.
    drop_fields = ["region_name", "region_name_id", "time_avg", "step"]

    feature_names = [f for f in grp_data.columns if f not in drop_fields]
    log(9, "Feature names: " + str(feature_names))
    log(9, "Creating a vector for regional data and models ...")

    model_count = 0
    all_skl_models = {}
    all_sizes_data = {}
    overall_start = time.time()

    log(2, "Training regression tree...")
    log(3, "Feature names: " + str(feature_names))
    for region in region_names:
        model_count += 1
        this_start = time.time()

        if one_big_tree:
            rd = grp_data
            region = "__ANY_REGION__"
        else:
            rd = grp_data[grp_data['region_name'] == region]

        if (rd.shape[0] < 1):
            log(4, "Region " + str(region) + " had no data.")
            continue

        print("")
        print("======================")
        print("--debug: region_name   = %s" % str(region))
        print("--debug: feature_names = %s" % str(feature_names))
        print("--debug: rd[]          = %s" % str(rd))
        print("----------------------")

        y = rd.time_avg
        x = rd[feature_names]

        reg = DecisionTreeRegressor(max_depth=tree_max_depth,random_state=0)

        reg.fit(x, y)

        with open("./output/models/rtree_latest/%s.dot" % region, "w") as dotfile:
            from sklearn import tree as _tree
            _tree.export_graphviz(reg, out_file=dotfile, feature_names=feature_names)

        all_skl_models[region] = reg
        this_elapsed = time.time() - this_start

        log(3, "regression[\"" + str(region) + "\"].x_shape" + "%-12s" % str(x.shape))

        if one_big_tree:
            break

    overall_elapsed = time.time() - overall_start
    log(2, "Done. Fit " + str(model_count) + " models in " + str(overall_elapsed) + " seconds.")

    return all_skl_models


    #region_name_dict = {}
    #for row in data.itertuples():
    #    key = (row.region_name)
    #    if key not in region_name_dict:
    #        region_name_dict[key] = str(row.region_name)
    #region_names = []
    #for entry in region_name_dict.items():
    #    key, value = entry
    #    region_names.append(value)


    #predictedTime = DecisionTreeRegressor()
    #predictedTime.fit(x, y)

    ##leafStdDev = np.std(y)

    #log(9, "predictedTime = " + str(predictedTime))
    #log(9, "predictions table:")
    #comp = ""
    #for row in x:
    #    for column in row:
    #        comp += ("[" + str(int(column)) + "]")
    #    comp += (" == " + str(predictedTime.predict(row.reshape(1, -1))))
    #    #comp += (" @ " + str(leafStdDev))
    #    #comp += (" @ " + str(pol_stds[row["policy_index"]))
    #    comp += "\n"
    #log(9, comp)

    ##with open("regress.dot", 'w') as dotfile:
    ##    from sklearn import tree as _tree
    ##    _tree.export_graphviz(predictedTime, out_file=dotfile, feature_names=feature_names)

    ##reg_tree = json.dumps(serializeRegressor(predictedTime), default=Obj2NativeTypes)
    ##with open("regress.json", 'w') as regfile:
    ##    regfile.write(reg_tree)

    #return reg_tree





#--------------------------------------------


def generateDecisionTree(log, data,
        assign_guid=0,
        tree_max_depth=2,
        one_big_tree=False):

    region_name_dict = {}
    for row in data.itertuples():
        key = (row.region_name)
        if key not in region_name_dict:
            region_name_dict[key] = str(row.region_name)
    region_names = []
    for entry in region_name_dict.items():
        key, value = entry
        region_names.append(value)

    log(3, "Creating categorical values for unique regions.")
    data["region_name"] = pd.Categorical(data["region_name"])
    data["region_name_id"] = data["region_name"].cat.codes
    log(3, "== CONTROLLER:  Generating name-swapping table.")
    name_swap = data[["region_name", "region_name_id"]]\
            .groupby(["region_name_id"], as_index=False, sort=True)\
            .first()

    # NOTE: How to create a column of binned values for a column:
    #data["op_count_binned"] = pd.qcut(data["op_count"].astype(float), 50, duplicates="drop")

    log(3, "Sorting, grouping, and pruning data.shape(" + str(data.shape) + ")")

    #
    #  NOTE: We feed in previously-grouped data, we don't do the aggregation here.
    #
    grp_data = data

    #start = time.time()
    #grp_data = data.groupby(
    #        ["region_name", "region_name_id", "num_elements", "policy_index"],
    #        as_index=False).agg({'time_avg':min})
    #elapsed_agg = time.time() - start
    #log(3, "data.groupby.agg(" + str(grp_data.shape) + ") took " + str(elapsed_agg) + " seconds.")

    # if (VERBOSE >= 9):
    #    log(9, "Grouped data:")
    #    tablePrint(grp_data.astype(str).values.tolist())
    #    log(9, "    " + str(grp_data.columns.astype(str).values.tolist()))

    # Get the translation table to go back and forth from categorical to actual
    # region name. We'll need this after doing the model.fit() step
    #if (VERBOSE >= 9):
    #    log(9, "Name swapping table:")
    #    tablePrint(name_swap[["region_name", "region_name_id"]].astype(str).values.tolist())

    # NOTE: We use time_avg instead of time_min/max/last because there will be
    #       different numbers of operations as we sweep through different
    #       dimensions/configs. We don't want to be picking some kernel
    #       just because it got a super low time doing only 1 operation.
    #       We want best on average.
    drop_fields = ["region_name", "region_name_id", "policy_index", "time_avg", "step"]

    feature_names = [f for f in grp_data.columns if f not in drop_fields]
    log(9, "Feature names: " + str(feature_names))
    log(9, "Creating a vector for regional data and models ...")

    model_count = 0
    all_skl_models = {}
    all_types_rule = {}
    all_rules_json = {}
    all_least_json = {}
    all_timed_json = {}
    all_sizes_data = {}
    overall_start = time.time()


    log(2, "Training...")
    for region in region_names:
        model_count += 1
        this_start = time.time()

        if one_big_tree:
            rd = grp_data
            region = "__ANY_REGION__"
        else:
            rd = grp_data[grp_data['region_name'] == region]

        if (rd.shape[0] < 1):
            continue

        element_minimum_to_evaluate_tree = -1
        # seq_winners =  rd[rd['policy_index']==1]
        # if (seq_winners.shape[0] > 0):
        #     element_minimum_to_evaluate_tree = seq_winners.max('num_elements').astype(int)

        #y = rd["policy_index"].astype(int)
        #x = rd.drop(drop_fields, axis="columns").values.astype(float)

        feature_cols = ['num_elements']

        y = rd.policy_index
        x = rd[feature_cols]

        #example = DecisionTreeClassifier(
        #         class_weight=None, criterion='gini', max_depth=6,
        #         max_features=x.shape[1], max_leaf_nodes=None,
        #         min_impurity_decrease=1e-07, min_samples_leaf=1,
        #         min_samples_split=2, min_weight_fraction_leaf=0.0,
        #         presort=False, random_state=None, splitter='best'))]

        clf = DecisionTreeClassifier(
                 class_weight=None, criterion='gini', max_depth=tree_max_depth,
                 min_samples_leaf=1, min_samples_split=2)

        # Conduct some model evaluation:
        x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=0.25, random_state=1) # 75% training and 25% test

        pipe = [('estimator', clf)]
        model = Pipeline(pipe)

        model.fit(x, y)

        trained_model = model.named_steps['estimator']
        y_pred = trained_model.predict(x_test)

        all_types_rule[region] = "DecisionTree"
        all_rules_json[region] = tree_to_data(trained_model, feature_names, name_swap, y)
        all_least_json[region] = element_minimum_to_evaluate_tree
        all_timed_json[region] = True
        all_sizes_data[region] = str(x.shape)
        all_skl_models[region] = trained_model

        this_elapsed = time.time() - this_start

        log(3, "model[\"" + str(region) + "\"].x_shape" + "%-12s" % str(x.shape) \
                + "%22s" % ("Acc%: " + "%6s" % ("%3.2f" % (100.0 * metrics.accuracy_score(y_test, y_pred)))))

        #dotfile = open("model.dot", 'w')
        #from sklearn import tree as _tree
        #_tree.export_graphviz(trained_model, out_file=dotfile, feature_names=feature_names)
        #dotfile.close()

        #print("")
        #print(json.dumps(tree_to_simple_str(trained_model, feature_names, name_swap, y), sort_keys=False, indent=4, ensure_ascii=True, default=Obj2NativeTypes))
        #print("")

        if one_big_tree:
            break

    overall_elapsed = time.time() - overall_start
    log(2, "Done. Fit " + str(model_count) + " models in " + str(overall_elapsed) + " seconds.")

    json_start = time.time()

    if one_big_tree == False:
        model_def = {
                "guid": assign_guid,
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
    else:
        model_def = {
                "guid": assign_guid,
                "driver": {
                    "rules": all_rules_json,
                    "least": all_least_json,
                    "timed": all_timed_json,
                    },
                "region_names": "__ANY_REGION__",
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
    if one_big_tree == False:
        model_def["region_names"].append("__ANY_REGION__")
        model_def["region_sizes"]["__ANY_REGION__"] = "(0, 0)"
        model_def["region_types"]["__ANY_REGION__"] = "Static"
        model_def["driver"]["rules"]["__ANY_REGION__"] = "0"
        model_def["driver"]["least"]["__ANY_REGION__"] = -1
        model_def["driver"]["timed"]["__ANY_REGION__"] = True

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4, ensure_ascii=True, default=Obj2NativeTypes) + "\n"
    json_elapsed = time.time() - json_start
    log(3, "Serializing models into JSON took " + str(json_elapsed) + " seconds.")

    return model_as_json, all_skl_models



def tree_to_data(decision_tree, feature_names=None, name_swap=None, y=None):
    def node_to_data(tree, node_id, criterion):
        if not isinstance(criterion, skl.tree.tree.six.string_types):
            criterion = "impurity"

        value = tree.value[node_id]
        if tree.n_outputs == 1:
            value = value[0, :]

        if tree.children_left[node_id] == skl.tree._tree.TREE_LEAF:
            return {
                "id": node_id,
                "criterion": criterion,
                "impurity": tree.impurity[node_id],
                "samples": tree.n_node_samples[node_id],
                "value": list(value),
                "class": int(decision_tree.classes_[np.argmax(value)])
            }
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




def tree_to_simple_str(decision_tree, feature_names=None, name_swap=None, y=None):
    def node_to_data(tree, node_id, criterion):
        if not isinstance(criterion, skl.tree.tree.six.string_types):
            criterion = "impurity"

        value = tree.value[node_id]
        if tree.n_outputs == 1:
            value = value[0, :]

        if tree.children_left[node_id] == skl.tree._tree.TREE_LEAF:
            return {
                "policy": decision_tree.classes_[np.argmax(value)]
            }
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
                "rule": "%s %s %s" % (feature, ruleType, ruleValue),
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


def Obj2NativeTypes(obj):
    if isinstance(obj, np.integer):
        return int(obj)
    elif isinstance(obj, np.floating):
        return float(obj)
    elif isinstance(obj, np.ndarray):
        return obj.tolist()
    elif isinstance(obj, datetime.datetime):
        return obj.__str__()


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



