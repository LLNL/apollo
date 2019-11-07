#!/usr/bin/env python
import os
import sys
import time
import json
import pickle

from multiprocessing import Process

import numpy as np
import pandas as pd
import pandasql as psql
import matplotlib.pyplot as plt

import apollo.trees as trees
import apollo.utils as utils

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
data_to_scan = pd.DataFrame()
data_to_group_for_learning = pd.DataFrame()

def no_log(level, msg):
    return

def main():
    global data
    data['path'] = '/g/g17/wood67/src/apollo/data/intel/001.node.001.rank'
    data['apollo.tracefile'] = 'trace.policy.0.csv'
    data['apollo.flushfile'] = 'intel.combined.out'
    data['apollo.stepsfile'] = 'steps.apollo.silent.csv'
    data['normal.stepsfile'] = 'steps.normal.silent.csv'
    data['policy.times.bestfile']  = 'policy.times.best.csv'
    data['policy.times.defaultfile'] = 'policy.times.default.csv'

    load_csv_data()

    project_model_over_trace()

    print("\nDone.")
    return



def project_model_over_trace():
    global data
    global data_to_scan
    # compile a model
    print("\nConstructing models.\n")
    all_models = construct_model_from_flush('apollo.flush')

    trace_pos = 0
    trace_pos_total = len(data['apollo.trace'].index)

    all_times = np.zeros((trace_pos_total,6))

    dfb = data['policy.times.best']
    dfd = data['policy.times.default']
    dff = data['apollo.flush']
    best_col_time    = dfb.columns.get_loc('min_time')
    best_col_policy  = dfb.columns.get_loc('policy_index')
    default_col_time = dfd.columns.get_loc('min_time')

    print("Size of input data sets:")
    print("\tdfb [policy.times.best] .....: %d" % len(dfb.index))
    print("\tdfd [policy.times.default] ..: %d" % len(dfd.index))
    print("\tdff [intel.combined.out] ....: %d" % len(dff.index))
    rt_total = 0.0
    rt_avg   = 0.0
    rt_left  = 0.0

    ### Gather some stats about our data set ###

    # Purpose:
    #       Look up the execution time for any given region/elem/policy
    #       combination. This is needed for predicting the runtime of
    #       policies that might get recommended by a model.
    most = {}
    for row in dff.itertuples():
        key = (row.region_name, row.num_elements, row.policy_index)
        if key not in most:
            most[key] = (row.time_avg, row.policy_index)
        else:
            cur_time, cur_idx = most[key]
            if cur_time > row.time_avg:
                most[key] = (row.time_avg, row.policy_index)

    # Purpose:
    #       Track the number of times the model ended up recommending
    #       each policy as the trace was evaluated.
    policy_recommended = np.zeros(20)

    # Purpose:
    #       Track the number of times a certain policy recommendation
    #       ended up not having a time in the exhaustive flush combination
    #       to look up when applying it to the trace, a calculated synthetic
    #       time per element for each policy, and the accumulated time
    #       contributed to the execution time based on num_elements
    #       multipled by synthetic time
    syntimes = {}
    data_to_scan = dff
    results = psql.sqldf("""
        SELECT
            policy_index,
            AVG(time_avg / num_elements) AS avg_time_per_elem
        FROM
            data_to_scan
        GROUP BY
            policy_index
        ;
        """, globals())
    for row in results.itertuples():
        key = row.policy_index
        if key not in syntimes:
            syntimes[key] = (0, row.avg_time_per_elem, 0, 0.0, 0.0)
        else:
            count, min_per_elem, elems, acc_time = syntimes[key]
            if (min_per_elem > row.avg_time_per_elem):
                syntimes[key] = (0, row.avg_time_per_elem, 0, 0.0, 0.0)


    # Purpose:
    #       Assemble a dictionary of the fastest performing loops from
    #       the integrated flush data. Walk through this to assess what
    #       loops are likely going to get recommended by the model.
    #       This is useful for assessing whether the model generation is
    #       biased or broken, or if the data set is simply strongly
    #       skewed to favor a particular policy.
    policy_fastest    = np.zeros(20)
    fast = {}
    for row in dff.itertuples():
        key = (row.region_name, row.num_elements)
        if key not in fast:
            fast[key] = (row.time_avg, row.policy_index)
        else:
            cur_time, cur_pol = fast[key]
            if cur_time > row.time_avg:
                fast[key] = (row.time_avg, row.policy_index)
    for entry in fast.items():
        key, value = entry
        cur_time, cur_pol = value
        policy_fastest[cur_pol] += 1

    # Purpose:
    #       This is an externally calculated "best-possible" run.
    #       TODO: Validate this compared to our own internal calculations.
    best = {}
    for row in dfb.itertuples():
        key = (row.region_name, row.num_elements)
        if key not in best:
            best[key] = (row.min_time, row.policy_index)
        else:
            cur_time, cur_index = best[key]
            if cur_time > row.min_time:
                best[key] = (row.min_time, row.policy_index)

    # Purpose:
    #       An externally filtered set of flush data for policy 0, the
    #       OpenMP defaults.
    deft = {}
    for row in dfd.itertuples():
        key = (row.region_name, row.num_elements)
        if key not in deft:
            deft[key] = row.min_time
        else:
            cur_time = deft[key]
            if cur_time > row.min_time:
                deft[key] = row.min_time

    print("\nSize of lookup dictionaries:")
    print("\tlen(best) .....: %d" % len(best))
    print("\tlen(deft) .....: %d" % len(deft))
    print("\tlen(most) .....: %d" % len(most))
    print("\tlen(fast) .....: %d" % len(fast))


    total_default = 0.0
    total_best    = 0.0
    total_trace   = 0.0
    total_model   = 0.0

    model_key_error   = 0
    best_key_error    = 0
    default_key_error = 0

    print("\nProcessing application trace:")
    for row in data['apollo.trace'].itertuples():
        rt_start = time.time()

        step          = int(row.step)
        region_name   = str(row.region_name)
        policy_index  = int(row.policy_index)
        num_threads   = int(row.num_threads)
        num_elements  = int(row.num_elements)
        time_exec     = float(row.time_exec)


        model_policy = int(all_models[region_name].predict([[num_elements]]))
        policy_recommended[model_policy] += 1
        try:
            model_time, cur_pol = most[(region_name, num_elements, model_policy)]
        except KeyError:
            st_count, st_avg_time_per_elem, st_elem_total, st_time_total, st_trace_time = syntimes[model_policy]
            model_time       = (num_elements * st_avg_time_per_elem)
            model_key_error += 1
            st_count        += 1
            st_elem_total   += num_elements
            st_time_total   += model_time
            st_trace_time   += time_exec
            syntimes[model_policy] = (st_count, st_avg_time_per_elem, st_elem_total, st_time_total, st_trace_time)

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
        sys.stdout.write("\t%7d of %-7d [%25s] %2.2f min. left.\r" % (
                (trace_pos - 1),
                trace_pos_total,
                progressBar(trace_pos, trace_pos_total, 25, fill='>'),
                rt_left))

        total_best    += best_time
        total_default += default_time
        total_trace   += row.time_exec
        total_model   += model_time

    #
    print(' '*80 + "\nTrace simulation is complete!\n")

    ### The main analysis / trace simulation loop is complete.
    ### Output the results...


    pyplot_process = None
    wait_for_pyplot_process = False
    if os.environ.get("DISPLAY") is None:
        print("NOTE: Skipping plot generation, your connection does not support a display.")
    else:
        print("Generating plot in the background.")
        def pyplot_worker():
            dft = pd.DataFrame(all_times,
                               columns=('trace_pos',
                                   'step',
                                   'time_exec',
                                   'time_best',
                                   'time_default',
                                   'time_model'))
            fig,ax = plt.subplots()
            dft.plot(x='trace_pos', y='time_exec', ax=ax, linestyle='-',
                    title="Cleverleaf Trace and Model Analysis")
            dft.plot(x='trace_pos', y='time_best', ax=ax, linestyle='--')
            dft.plot(x='trace_pos', y='time_model', ax=ax, linestyle='-.')
            dft.plot(x='trace_pos', y='time_default', ax=ax, linestyle=':')
            ax.legend(["trace", "best", "model", "default"], loc='upper right')
            ax.tick_params(axis='y', which='minor', left=True)
            ax.tick_params(axis='x', which='minor', bottom=True)
            plt.grid(True)
            plt.xlabel('Cleverleaf Trace Progress')
            plt.ylabel('Time (seconds)')
            plt.savefig('trace_times.png')
            #plt.show()
            return
        pyplot_process = Process(target=pyplot_worker, args=())
        pyplot_process.start()
        wait_for_pyplot_process = True


    print("Exporting .CSV file.")
    with open('trace_times.csv', 'w') as f:
        f.write("trace_pos,step,time_exec,best_time,default_time,model_time\n")
        for row in all_times:
            f.write("%d,%d,%1.12f,%1.12f,%1.12f,%1.12f\n" %
                    (row[0], row[1], row[2], row[3], row[4], row[5]))




    total_recommendations = 0
    total_count_of_fastest = 0
    print("\nPolicy recommendation count               |   Flush entries being fastest:")
    print("------------------------------------------+------------------------------------------")
    for i in range(0, 20):
        print(" [%2d]: %8d  %21s    |  [%2d]: %8d  %21s" % (
            i, policy_recommended[i],
            progressBar(policy_recommended[i], trace_pos_total, 21, fill='#'),
            i, policy_fastest[i],
            progressBar(policy_fastest[i], len(best), 21, fill='#')))
        total_recommendations += policy_recommended[i]
        total_count_of_fastest += policy_fastest[i]
    print("------------------------------------------+------------------------------------------")
    print("Total: %-8d                           | Total: %-8d\n" % (
        total_recommendations, total_count_of_fastest))

    # Done with the principle part of the simulation.
    # Emit any warnings last so that they show up very clearly.

    if total_recommendations != trace_pos_total:
        print("WARNING: The number of recommendations for all policies\n" + \
              "         does not match the number of loops encountered!")

    if total_count_of_fastest != len(best):
        print("WARNING: The number of fastest policies counted when scanning\n" + \
              "         the integrated flush file does not match the size of the\n" + \
              "         data set used for the best policy times!")


    if ((best_key_error > 0) or (model_key_error > 0) or (default_key_error > 0)):
        print("WARNING: There were key errors (unfound region/elem combinations)\n" + \
              "         when looking up execution times in the dictionaries!\n" + \
              "         This will result in predicted runtimes being slightly skewed.\n")
        print("\t best .....: %d\n\t model ....: %d\n\t default ..: %d\n" % \
            (best_key_error, model_key_error, default_key_error))
        total_synthetic_time = 0.0
        if model_key_error > 0:
            print("Because there were model key errors, the following 'synthetic times' were used:\n")
            print("policy  ( avg_per_elem  * total_num_elems ) == synthetic_time")
            print("---------------------------------------------------------------------------------------")
            for i in range(0, 20):
                st_count, st_avg_time_per_elem, st_elem_total, st_time_total, st_trace_time = syntimes[i]
                if st_count > 0:
                    print(" [%2d]   ( %4.16f  *  %-8d ) == %4.12f    (%4.12f trace)" % (
                        i, st_avg_time_per_elem, st_elem_total,
                        st_time_total, st_trace_time))
                    total_synthetic_time  += st_time_total
            print("---------------------------------------------------------------------------------------")
            print("                                        Total: %4.12f" % total_synthetic_time)
    print("\n\n\t--- Final simulated execution times: ---\n")
    print(("\tRaw trace time ..............: %4.12f\n" + \
          "\tOpenMP defaults .............: %4.12f\n" + \
          "\tBest-possible policy ........: %4.12f\n" + \
          "\tModel-recommended policy ....: %4.12f\n") % \
          (total_trace, total_default, total_best, total_model))


    if wait_for_pyplot_process == True:
        print("Waiting for PyPlot subprocess to complete...")
        pyplot_process.join()
        print("")

    return


#------------
def progressBar(amount, total, length, fill='='):
    if amount >= total:
        return fill * length
    if length < 4: length = 4
    fillLen = int(length * amount // total)
    emptyLen = length - 1 - fillLen
    bar = (fill * fillLen) + fill + (" " * emptyLen)
    return bar


def construct_model_from_flush(flush_key):
    global data

    training_data = group_flush_data(flush_key)

    combined_model_json, all_skl_models = \
            trees.generateDecisionTree(
                    no_log,
                    training_data,
                    assign_guid=0,
                    tree_max_depth=2)

    return all_skl_models


def group_flush_data(dfkey):
    global data
    global data_to_group_for_learning
    data_to_group_for_learning = data[dfkey]
    sql_string = """\
        SELECT
            region_name, policy_index, step, num_elements, MIN(time_avg) AS time_avg
        FROM
            data_to_group_for_learning
        GROUP BY
            region_name, num_elements, step
        ;
        """
    return psql.sqldf(sql_string, globals())



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
    global data_to_group_for_learning

    def load_and_report(data, dfkey, csvfile):
        data[dfkey] = pd.read_csv(data['path'] + '/' + csvfile)
        dfbytes = data[dfkey].memory_usage(index=False, deep=True).sum()
        print("       data[%22s]   %10s   %s" % (dfkey, format_bytes(dfbytes), csvfile))
        return data
    ###
    print("\nData source:\n\t%s\n" % data['path'])
    print("Loading:")
    data = load_and_report(data, 'apollo.trace', data['apollo.tracefile'])
    data = load_and_report(data, 'apollo.flush', data['apollo.flushfile'])
    data = load_and_report(data, 'apollo.steps', data['apollo.stepsfile'])
    data = load_and_report(data, 'normal.steps', data['normal.stepsfile'])
    data = load_and_report(data, 'policy.times.best',    data['policy.times.bestfile'])
    data = load_and_report(data, 'policy.times.default', data['policy.times.defaultfile'])
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
