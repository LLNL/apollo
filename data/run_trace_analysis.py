#!/usr/bin/env python
import os
import sys
import time
import json
import pickle
import threading

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

import sklearn as skl
from sklearn                 import metrics
from sklearn.preprocessing   import StandardScaler
from sklearn.tree            import DecisionTreeClassifier
from sklearn.tree            import DecisionTreeRegressor
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.model_selection import train_test_split
from sklearn.svm             import SVC

data = {}

def main():
    global data
    data['path'] = '/g/g17/wood67/src/apollo/data/intel/001.node.001.rank'
    data['apollo.tracefile'] = 'trace.policy.0.csv'
    data['apollo.flushfile'] = 'flush.grouped.csv'
    data['apollo.stepsfile'] = 'steps.apollo.silent.csv'
    data['normal.stepsfile'] = 'steps.normal.silent.csv'
    data['policy.times.bestfile']  = 'policy.times.best.csv'
    data['policy.times.defaultfile'] = 'policy.times.default.csv'

    load_csv_data()

    #trace_thread = threading.Thread(
    #    target=project_model_over_trace,
    #    args=(data))
    #trace_thread.start()

    project_model_over_trace()

    print("\nDone.")
    return





def project_model_over_trace():
    global data
    # compile a model
    print("Constructing models.")
    all_models = construct_model_from_flush('apollo.flush')

    print("Processing trace.\n")

    trace_pos = 0
    trace_pos_total = len(data['apollo.trace'].index)

    all_times = np.zeros((trace_pos_total,6))

    dfb = data['policy.times.best']
    dfd = data['policy.times.default']
    dff = data['apollo.flush']
    best_col_time    = dfb.columns.get_loc('min_time')
    best_col_policy  = dfb.columns.get_loc('policy_index')
    default_col_time = dfd.columns.get_loc('min_time')

    print("dfb: %d" % len(dfb.index))
    print("dfd: %d" % len(dfd.index))

    rt_total = 0.0
    rt_avg   = 0.0
    rt_left  = 0.0


    # Set up accelleration structure for searching the known times:
    most = {}
    for row in dff.itertuples():
        key = (row.region_name, row.num_elements, row.policy_index)
        if key not in most:
            most[key] = row.time_avg
        else:
            cur_time = most[key]
            if cur_time > row.time_avg:
                most[key] = row.time_avg

    best = {}
    for row in dfb.itertuples():
        key = (row.region_name, row.num_elements)
        if key not in best:
            best[key] = (row.min_time, row.policy_index)
        else:
            cur_time, cur_index = best[key]
            if cur_time > row.min_time:
                best[key] = (row.min_time, row.policy_index)
    deft = {}
    for row in dfd.itertuples():
        key = (row.region_name, row.num_elements)
        if key not in deft:
            deft[key] = row.min_time
        else:
            cur_time = deft[key]
            if cur_time > row.min_time:
                deft[key] = row.min_time

    total_default = 0.0
    total_best    = 0.0
    total_trace   = 0.0
    total_model   = 0.0

    model_key_error = 0
    best_key_error = 0
    default_key_error = 0

    for row in data['apollo.trace'].itertuples():


        rt_start = time.time()

        step          = int(row.step)
        region_name   = str(row.region_name)
        policy_index  = int(row.policy_index)
        num_threads   = int(row.num_threads)
        num_elements  = int(row.num_elements)
        time_exec     = float(row.time_exec)

        model_policy = int(all_models[region_name].predict([[num_elements]]))
        try:
            model_time   = most[(region_name, num_elements, model_policy)]
        except KeyError:
            model_key_error += 1
            model_time = 0.0

        try:
            best_time, best_policy = best[(region_name, num_elements)]
        except KeyError:
            best_key_error += 1
            best_time, best_policy = (0.0, -1.0)
        try:
            default_time = deft[(region_name, num_elements)]
        except KeyError:
            default_key_error += 1
            default_time = 0.0

        all_times[trace_pos][0] = trace_pos
        all_times[trace_pos][1] = step
        all_times[trace_pos][2] = time_exec
        all_times[trace_pos][3] = best_time
        all_times[trace_pos][4] = default_time
        all_times[trace_pos][5] = model_time

        trace_pos += 1

        rt_stop   = time.time()
        rt_total += (rt_stop - rt_start)
        rt_avg    = (rt_total / trace_pos)
        rt_left   = 0.01667 * (rt_avg * (trace_pos_total - trace_pos))
        sys.stdout.write("%7d of %7d [%25s] %2.2f min. left.\r" % (
                (trace_pos - 1),
                trace_pos_total,
                progressBar(trace_pos, trace_pos_total, 25, fill='>'),
                rt_left))

        total_best += best_time
        total_default += default_time
        total_trace += row.time_exec
        total_model += model_time

    #
    print(' '*80)
    print("\nExporting .CSV file.")
    with open('trace_times.csv', 'w') as f:
        f.write("trace_pos,step,time_exec,best_time,default_time\n")
        for row in all_times:
            f.write("%d,%d,%1.8f,%1.8f,%1.8f\n" % \
                    (row[0], row[1], row[2], row[3], row[4]))

    print("\ttrace: %1.8f\n\tdefault: %1.8f\n\tbest: %1.8f\n\tmodel: %1.8f\n" % \
        (total_trace, total_default, total_best, total_model))

    print("key errors:\n\tb: %d\n\tm: %d\n\td: %d\n" % \
            (best_key_error, model_key_error, default_key_error))

    print("Generating plot.")
    dft = pd.DataFrame(all_times,
                       columns=('trace_pos', 'step', 'time_exec', 'time_best', 'time_default', 'time_model'))
    fig,ax = plt.subplots()

    fig.set_size_inches(8, 3, forward=True)
    dft.plot(x='trace_pos', y='time_exec', ax=ax, linestyle='-', title="Cleverleaf Trace and Model Analysis")
    dft.plot(x='trace_pos', y='time_best', ax=ax, linestyle='--')
    dft.plot(x='trace_pos', y='time_model', ax=ax, linestyle='-.')
    dft.plot(x='trace_pos', y='time_default', ax=ax, linestyle=':')
    ax.legend(["trace", "best", "default"], loc='upper right')
    ax.tick_params(axis='y', which='minor', left=True)
    ax.tick_params(axis='x', which='minor', bottom=True)
    plt.grid(True)
    plt.xlabel('Cleverleaf Trace Progress')
    plt.ylabel('Time (seconds)')
    plt.savefig('trace_times.png')
    plt.show()
    return


#------------
def progressBar(amount, total, length, fill='='):
    if amount >= total:
        return fill * length
    if length < 4: length = 4
    fillLen = int(length * amount // total)
    emptyLen = length - 1 - fillLen
    bar = (fill * fillLen) + ">" + (" " * emptyLen)
    return bar


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
                "class": decision_tree.classes_[np.argmax(value)]
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
                "class": decision_tree.classes_[np.argmax(value)]
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


def construct_model_from_flush(flush_key):
    global data
    # Grab the table
    td = data[flush_key]

    # Make the data like the query for online learning
    # TODO

    # Filter the refined data
    td['region_name'] = pd.Categorical(td['region_name'])
    td['region_name_id'] = td['region_name'].cat.codes
    #
    name_swap = td[['region_name', 'region_name_id']]\
            .groupby(['region_name', 'region_name_id'], as_index=False, sort=True)\
            .first()

    grp_td = td.groupby(by=['region_name', 'region_name_id', 'num_elements', 'policy_index'],
                            as_index=False).agg({
                                'time_avg':'min'
                            })

    region_names = td['region_name'].unique().tolist()
    unique_policies = grp_td['policy_index'].unique().tolist()

    drop_fields = ['region_name', 'region_name_id', 'policy_index', 'time_avg']

    feature_names = [f for f in grp_td.columns if f not in drop_fields]
    model_count = 0

    # Set up the SKL pipeline
    # Build a model for each region
    all_skl_models = {}
    all_types_rule = {}
    all_rules_json = {}
    all_least_json = {}
    all_timed_json = {}
    all_sizes_data = {}

    one_big_tree = False

    for region in region_names:
        model_count += 1

        if one_big_tree:
            rd = grp_td
            region = "__ANY_REGION__"
        else:
            rd = grp_td[grp_td['region_name'] == region]

        if (rd.shape[0] < 1):
            continue

        y = rd['policy_index'].astype(int)
        x = rd.drop(drop_fields, axis="columns").values.astype(float)

        #example = DecisionTreeClassifier(
        #         class_weight=None, criterion='gini', max_depth=6,
        #         max_features=x.shape[1], max_leaf_nodes=None,
        #         min_impurity_decrease=1e-07, min_samples_leaf=1,
        #         min_samples_split=2, min_weight_fraction_leaf=0.0,
        #         presort=False, random_state=None, splitter='best'))]

        clf = DecisionTreeClassifier(
                 class_weight=None, criterion='gini', max_depth=2,
                 min_samples_leaf=1, min_samples_split=2)

        # Conduct some model evaluation:
        x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=0.25, random_state=1) # 75% training and 25% test

        pipe = [('estimator', clf)]
        model = Pipeline(pipe)

        model.fit(x, y)

        trained_model = model.named_steps['estimator']
        y_pred = trained_model.predict(x_test)

        # Does not work for small splits:
        #scores = cross_val_score(model, x, y, cv=5)

        all_types_rule[region] = "DecisionTree"
        all_rules_json[region] = tree_to_data(trained_model, feature_names, name_swap, y)
        all_least_json[region] = -1
        all_timed_json[region] = True
        all_sizes_data[region] = str(x.shape)
        all_skl_models[region] = trained_model

        #print("model[\"" + str(region) + "\"].x_shape" + "%-12s" % str(x.shape) \
        #        + ".y_shape" + "%-12s" % str(y.shape) \
        #        + "%22s" % ("Acc%: " + "%6s" % ("%3.2f" % (100.0 * metrics.accuracy_score(y_test, y_pred)))))

        if one_big_tree:
            #print("")
            #print(tree_to_simple_str(trained_model, feature_names, name_swap, y))
            #print("")
            break

    #
    # Now we're done building models.
    #
    if one_big_tree == False:
        model_def = {
                "guid": 0,
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
                "guid": 0,
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

    # Add in a default model (Static, OMP defaults) for any unnamed region:
    if one_big_tree == False:
        model_def["region_names"].append("__ANY_REGION__")
        model_def["region_sizes"]["__ANY_REGION__"] = "(0, 0)"
        model_def["region_types"]["__ANY_REGION__"] = "Static"
        model_def["driver"]["rules"]["__ANY_REGION__"] = "0"
        model_def["driver"]["least"]["__ANY_REGION__"] = "-1"
        model_def["driver"]["timed"]["__ANY_REGION__"] = True

    #model_as_json = json.dumps(model_def, sort_keys=False, indent=4, ensure_ascii=True) + "\n"

    return all_skl_models


def hide_traceback(exc_tuple=None, filename=None, tb_offset=None,
                   exception_only=False, running_compiled_code=False):
    import sys
    ipython = get_ipython()
    etype, value, tb = sys.exc_info()
    return ipython._showtraceback(etype, value, ipython.InteractiveTB.get_exception_only(etype, value))

def format_bytes(size):
    # 2**10 = 1024
    power = 2**10
    n = 0
    power_labels = {0 : '', 1: 'k', 2: 'M', 3: 'G', 4: 'T'}
    while size > power:
        size /= power
        n += 1
    return str("%3.3f " % size + power_labels[n] + 'B')

def load_csv_data():
    global data
    def load_and_report(data, dfkey, csvfile):
        data[dfkey] = pd.read_csv(data['path'] + '/' + csvfile)
        dfbytes = data[dfkey].memory_usage(index=False, deep=True).sum()
        print("       data[%s]   %10s   %s" % (dfkey, format_bytes(dfbytes), csvfile))
        return data
    ###
    print("Data source:\n\t%s\n" % data['path'])
    print("Loading:")
    data = load_and_report(data, 'apollo.trace', data['apollo.tracefile'])
    data = load_and_report(data, 'apollo.flush', data['apollo.flushfile'])
    data = load_and_report(data, 'apollo.steps', data['apollo.stepsfile'])
    data = load_and_report(data, 'normal.steps', data['normal.stepsfile'])
    data = load_and_report(data, 'policy.times.best',    data['policy.times.bestfile'])
    data = load_and_report(data, 'policy.times.default', data['policy.times.defaultfile'])
    print("")
    return data



    #print(
    #"""
    #            \     _ \   _ \  |     |      _ \\
    #           _ \   |   | |   | |     |     |   |
    #          ___ \  ___/  |   | |     |     |   |
    #        _/    _\_|    \___/ _____|_____|\___/

    #       -  -  -  --  --  ---  --= --== ==*# ###>\n\n
    #""")


if __name__ == "__main__":
    main()
