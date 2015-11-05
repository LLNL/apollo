import sys
import argparse

import argparse
import warnings

from sklearn.tree import DecisionTreeClassifier
from sklearn.pipeline import Pipeline

import rlsl
from rlsl.pipeline import get_pipeline_steps
from rlsl.transformers import DataframePipeline, AutoDataFrameMapper

from rlsl.codegen import CodeGenerator
from rlsl.util.timer import Timer

description = "Build and install packages"

def setup_parser(subparser):
    subparser.add_argument(
        '-v', '--verbose', action='store_true', dest='verbose',
        help="Display verbose build output while installing.")
    subparser.add_argument(
        '-p', '--predict', action='store', dest='predict',
        help="Select which label to predict for: policy or thread")
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="specs of packages to install")


def gen_code(X, y, labelencoder):
    adf = AutoDataFrameMapper()

    pipeline = Pipeline([
        ('mapper', adf),
        ('clf', DecisionTreeClassifier(max_leaf_nodes=15))])
    features = adf.get_feature_list(X)
    pipeline.fit(X,y)
    CodeGenerator().get_code(pipeline.steps[-1][1], features,
                             labelencoder.get_labels())


def gen(parser, args):
    if not args.files:
        sys.stderr.write("gen requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = rlsl.util.loader.load(app_data_name, instruction_data_name)

    pipeline = DataframePipeline(get_pipeline_steps(
        kind='policy', data=instruction_data,
        dropped_features=rlsl.dropped_features + list(instruction_data)))
        #dropped_features=rlsl.dropped_features))

    X, y = pipeline.fit_transform(app_data)
    gen_code(X, y, pipeline['y'])
