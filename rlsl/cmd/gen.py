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
        '-p', '--predict', action='store', dest='predict',
        help="Select which label to predict for: policy or thread")
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="files containing application samples and instruction data")


def gen_code(app_data, instruction_data, kind, features, interactive=True, keep_features=False, instructions=False):
    if keep_features:
        #print features
        all_features = list(app_data) + list(instruction_data)
        if instructions:
            steps = get_pipeline_steps(
                kind=kind, data=instruction_data,
                dropped_features=[x for x in all_features if x not in features or x in list(instruction_data)])
        else:
            steps = get_pipeline_steps(
                kind=kind, data=instruction_data,
                dropped_features=[x for x in all_features if x not in features])
    else:
        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=getattr(rlsl, features))

    pipeline = DataframePipeline(steps)

    #pipeline = DataframePipeline(get_pipeline_steps(
    #    kind='policy', data=instruction_data,
    #    dropped_features=rlsl.dropped_features + list(instruction_data)))
    #    #dropped_features=rlsl.dropped_features))

    X, y = pipeline.fit_transform(app_data)
    labelencoder = pipeline['y']

    adf = AutoDataFrameMapper()

    pipeline = Pipeline([
        ('mapper', adf),
        ('clf', DecisionTreeClassifier(max_depth=3))])
    features = adf.get_feature_list(X)
    pipeline.fit(X,y)
    model = CodeGenerator(kind).get_code(
        pipeline.steps[-1][1],
        features,
        labelencoder.get_labels(),
        list(instruction_data),
    )

    if interactive:
        print model
    else:
        return model

def grab_tree(app_data, instruction_data, kind, features, interactive=True, keep_features=False):
    if keep_features:
        #print features
        all_features = list(app_data) + list(instruction_data)
        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=[x for x in all_features if x not in features])
    else:
        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=getattr(rlsl, features))

    pipeline = DataframePipeline(steps)
    X, y = pipeline.fit_transform(app_data)
    labelencoder = pipeline['y']

    adf = AutoDataFrameMapper()

    pipeline = Pipeline([
        ('mapper', adf),
        ('clf', DecisionTreeClassifier(max_leaf_nodes=4))])
    features = adf.get_feature_list(X)
    pipeline.fit(X,y)

    model = CodeGenerator(kind).get_code(pipeline.steps[-1][1], features,
                             labelencoder.get_labels())
    return model


def gen(parser, args):
    if not args.files:
        sys.stderr.write("gen requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = rlsl.util.loader.load(app_data_name, instruction_data_name)

    get_model(app_data, instruction_data, args.kind, args.features)

