import argparse
import warnings

import numpy as np
import pandas as pd

from sklearn.tree import DecisionTreeClassifier
from sklearn.linear_model import SGDClassifier
from sklearn.pipeline import Pipeline
from sklearn.cross_validation import cross_val_score
from sklearn.decomposition import PCA

import artemis
from artemis.pipeline import get_pipeline_steps
from artemis.transformers import DataframePipeline, AutoDataFrameMapper

from artemis.util.loader import PandasCaliperLoader, PandasInstructionLoader
from artemis.codegen import CodeGenerator
from artemis.util.timer import Timer

description = "Model..."

CLASSIFIERS = [
    'DecisionTreeClassifier',
]

clfs = []

for c in CLASSIFIERS:
    clfs.append(globals()[c]())


def setup_parser(subparser):
    subparser.add_argument(
        '-p', '--predict', action='store', dest='predict',
        help="Select which label to predict for: policy or thread")
    subparser.add_argument(
        '-f', '--features', action='store', dest='features',
        default='dropped_features',
        help="Feature set to use for predictions")
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="files containing application samples and instruction data")


def run_model(app_data, instruction_data, kind, features, interactive=True, keep_features=False, **kwargs):
    if keep_features:
        all_features = list(app_data) + list(instruction_data)
        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=[x for x in all_features if x not in features])
    else:

        print features

        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=getattr(artemis, features))

    steps = [x for x in steps if x[0] != 'threads']

    pipeline = DataframePipeline(steps)

    X, y = pipeline.fit_transform(app_data)


    #print X['loop']

    pipeline = Pipeline([
        ('mapper', AutoDataFrameMapper()),
        #('pca', PCA(n_components=2)),
        #('pca', PCA(n_components=3)),
        ('clf', DecisionTreeClassifier(max_depth=kwargs.get('depth')))])
        #('clf', SGDClassifier())])
        #('clf', DecisionTreeClassifier())])
        #('clf', DecisionTreeClassifier(max_depth=3))])

    scores = cross_val_score(pipeline, X, y, cv=5)

    if interactive:
        print scores.mean()
    else:
        return scores.mean()

def run_cross_app_model(train_app, train_inst, test_app, test_inst, kind, features):
    train_inst_set = set(list(train_inst))
    test_inst_set = set(list(test_inst))

    train_app_set = set(list(train_app))
    test_app_set = set(list(test_app))

    for inst in train_inst_set - test_inst_set:
        test_inst[inst] = 0
    for inst in test_inst_set - train_inst_set:
        train_inst[inst] = 0

    feats_in_both = train_app_set & test_app_set

    train_app = train_app[list(feats_in_both)]
    test_app = test_app[list(feats_in_both)]

    steps = get_pipeline_steps(
        kind=kind, data=train_inst,
        dropped_features=getattr(artemis, features))
    pipeline = DataframePipeline(steps)

    X_train, y_train = pipeline.fit_transform(train_app)

    steps = get_pipeline_steps(
        kind=kind, data=test_inst,
        dropped_features=getattr(artemis, features))
    pipeline = DataframePipeline(steps)

    X_test, y_test = pipeline.fit_transform(test_app)

    pipeline = Pipeline([
        ('mapper', AutoDataFrameMapper()),
        ('clf', DecisionTreeClassifier())])

    pipeline.fit_transform(X_train, y_train)

    test_size = min(len(X_train), len(X_test))
    print "Test size is %d" % test_size

    y_pred = pipeline.predict(X_test[:test_size])

    results = (y_pred == y_test[:test_size])
    if True in results:
        return pd.value_counts(results)[True]/float(len(results))
    else:
        return 0.0


def model(parser, args):
    warnings.simplefilter("ignore")

    if not args.files:
        sys.stderr.write("install requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = artemis.util.loader.load(app_data_name, instruction_data_name)

    run_model(app_data, instruction_data, args.predict, args.features)
