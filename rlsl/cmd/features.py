import argparse
import warnings

import numpy as np
import pandas as pd

from sklearn.ensemble import RandomForestClassifier
from sklearn.pipeline import Pipeline

import rlsl
from rlsl.pipeline import get_pipeline_steps
from rlsl.transformers import DataframePipeline, AutoDataFrameMapper

from rlsl.codegen import CodeGenerator
from rlsl.util.timer import Timer


def setup_parser(subparser):
    subparser.add_argument(
        '-v', '--verbose', action='store_true', dest='verbose',
        help="Display verbose build output while installing.")
    subparser.add_argument(
        '-p', '--predict', action='store', dest='predict',
        help="Select which label to predict for: policy or thread")
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="specs of packages to install")


def get_features(X, y):
    pipeline = Pipeline([
        ('mapper', AutoDataFrameMapper()),
        ('clf', RandomForestClassifier())])

    pipeline.fit(X,y)

    features = AutoDataFrameMapper().get_feature_list(X)
    forest = pipeline.steps[-1][1]
    importances = forest.feature_importances_
    std = np.std([tree.feature_importances_ for tree in forest.estimators_],
                axis=0)
    indices = np.argsort(importances)[::-1]

    features = [features[indices[f]] for f in range(min(15, len(features)))]
    importances = [importances[indices[f]] for f in range(min(15, len(features)))]

    return features, importances


def features(parser, args):
    warnings.simplefilter("ignore")

    if not args.files:
        sys.stderr.write("install requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = rlsl.util.loader.load(app_data_name, instruction_data_name)

    steps = get_pipeline_steps(
        kind=args.predict, data=instruction_data,
        dropped_features=rlsl.dropped_features)

    steps = [x for x in steps if x[0] != 'threads']

    pipeline = DataframePipeline(steps)
    X, y = pipeline.fit_transform(app_data)

    features, importances = get_features(X, y)

    print("Feature ranking:")
    for count,(f,i) in enumerate(zip(features, importances)):
        print("%d. feature %s (%f)" % (count, f, i))
