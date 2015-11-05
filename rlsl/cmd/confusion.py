import argparse
import warnings

import numpy as np
import pandas as pd

from sklearn.tree import DecisionTreeClassifier
from sklearn.pipeline import Pipeline
from sklearn.cross_validation import cross_val_score
from sklearn.decomposition import PCA
from sklearn.metrics import confusion_matrix

import rlsl
from rlsl.pipeline import get_pipeline_steps
from rlsl.transformers import DataframePipeline, AutoDataFrameMapper

from rlsl.util import get_train_test_inds

from rlsl.util.loader import PandasCaliperLoader, PandasInstructionLoader
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
        '-f', '--features', action='store', dest='features',
        default='dropped_features',
        help="Feature set to use for predictions")
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="specs of packages to install")


def confusion(parser, args):
    warnings.simplefilter("ignore")

    if not args.files:
        sys.stderr.write("install requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = rlsl.util.loader.load(app_data_name, instruction_data_name)

    steps = get_pipeline_steps(
        kind=args.predict, data=instruction_data,
        dropped_features=getattr(rlsl, args.features))

    steps = [x for x in steps if x[0] != 'threads']

    pipeline = DataframePipeline(steps)

    X, y = pipeline.fit_transform(app_data)

    pipeline = Pipeline([
        ('mapper', AutoDataFrameMapper()),
        #('pca', PCA(n_components=2)),
        #('pca', PCA(n_components=3)),
        ('clf', clfs[0])])

    train_inds, test_inds = get_train_test_inds(y)

    pipeline.fit(X[train_inds], y[train_inds])
    results = pipeline.predict(X[test_inds])

    cm = confusion_matrix(y[test_inds], results)
    print cm.astype('float') / cm.sum(axis=1)[:, np.newaxis]
