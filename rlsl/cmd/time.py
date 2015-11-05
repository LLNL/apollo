import argparse
import warnings

import numpy as np
import pandas as pd

from sklearn.tree import DecisionTreeClassifier
from sklearn.pipeline import Pipeline
from sklearn.cross_validation import cross_val_score

import rlsl
from rlsl.pipeline import get_pipeline_steps
from rlsl.transformers import DataframePipeline, AutoDataFrameMapper

from rlsl.util.loader import PandasCaliperLoader, PandasInstructionLoader
from rlsl.util import get_train_test_inds
from rlsl.codegen import CodeGenerator
from rlsl.util.timer import Timer

description = "Model..."

CLASSIFIERS = [
    'DecisionTreeClassifier',
]

clfs = []


for c in CLASSIFIERS:
    clfs.append(globals()[c]())


def setup_parser(subparser):
    subparser.add_argument(
        '-v', '--verbose', action='store_true', dest='verbose',
        help="Display verbose build output while installing.")
    subparser.add_argument(
        '-p', '--predict', action='store', dest='predict',
        help="Select which label to predict for: policy or thread")
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="specs of packages to install")


def get_optimal_time(X):
    print X.groupby('problem_size')['time.duration'].sum()


def get_time(data, Xf, X, y, labelencoder, kind=None):
    train_inds, test_inds = get_train_test_inds(y)

    for i,c  in enumerate(CLASSIFIERS):
        pipeline = Pipeline([
            ('mapper', AutoDataFrameMapper()),
            ('clf', clfs[i])])

        pipeline.fit(X[train_inds], y[train_inds])

        results = pipeline.predict(X)

        resultlabels = labelencoder.get_encoder().inverse_transform(results)
        test_df = Xf[['problem_size', 'loop_count', 'seg_it', 'seg_exec']]

        if 'policy' in kind:
            test_df['seg_it'] = map(lambda x: x.split(' ')[0], resultlabels)
            test_df['seg_exec'] = map(lambda x: x.split(' ')[1], resultlabels)
            merged_test_df = pd.merge(test_df, data[['loop_count', 'problem_size', 'seg_it', 'seg_exec', 'time.duration']], on=['problem_size', 'loop_count'], how='left')
            mdf = merged_test_df[(merged_test_df['seg_exec_x'] == merged_test_df['seg_exec_y']) & (merged_test_df['seg_it_x'] == merged_test_df['seg_it_y'])]

        elif 'thread' in kind:
            test_df['num_threads'] = resultlabels
            merged_test_df = pd.merge(test_df, data[['loop_count', 'problem_size',
                                                            'num_threads',
                                                            'seg_it', 'seg_exec',
                                                            'time.duration']],
                                    on=['problem_size', 'loop_count', 'seg_it', 'seg_exec'], how='inner')
            mdf = merged_test_df[(merged_test_df['num_threads_x'] == merged_test_df['num_threads_y'])]

        print mdf.groupby('problem_size')['time.duration'].sum()


def do_time(app_data, instruction_data, kind):
    pipeline = DataframePipeline(get_pipeline_steps(
        kind=kind, data=instruction_data,
        dropped_features=rlsl.dropped_features))

    X, y = pipeline.fit_transform(app_data)
    get_optimal_time(pipeline.get_x('drop features'))
    get_time(pipeline.get_x('duplicates'), pipeline.get_x('drop features'), X, y, pipeline['y'], kind=kind)


def time(parser, args):
    warnings.simplefilter("ignore")

    if not args.files:
        sys.stderr.write("time requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = rlsl.util.loader.load(app_data_name, instruction_data_name)

    do_time(app_data, instruction_data, args.predict)
