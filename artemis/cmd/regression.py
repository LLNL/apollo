import argparse
import warnings

from sklearn.linear_model import SGDRegressor, BayesianRidge, Lasso
from sklearn.pipeline import Pipeline
from sklearn.cross_validation import cross_val_score, cross_val_predict

import artemis
from artemis.pipeline import get_pipeline_steps
from artemis.transformers import DataframePipeline, AutoDataFrameMapper

from artemis.util.loader import PandasCaliperLoader, PandasInstructionLoader
from artemis.codegen import CodeGenerator


description = "Regression..."


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


def run_regression(app_data, instruction_data, kind, features, interactive=True, keep_features=False, **kwargs):
    if keep_features:
        all_features = list(app_data) + list(instruction_data)
        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=[x for x in all_features if x not in features])
    else:
        steps = get_pipeline_steps(
            kind=kind, data=instruction_data,
            dropped_features=getattr(artemis, features))

    steps = [x for x in steps if x[0] != 'threads']

    pipeline = DataframePipeline(steps)
    X, y = pipeline.fit_transform(app_data)

    pipeline = Pipeline([
        ('mapper', AutoDataFrameMapper()),
        ('clf', Lasso())])

    scores = cross_val_predict(pipeline, X, y)

    return y, scores

    #if interactive:
    #    print scores.mean()
    #else:
    #    return scores.mean()


def regression(parser, args):
    warnings.simplefilter("ignore")

    if not args.files:
        sys.stderr.write("install requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = artemis.util.loader.load(app_data_name, instruction_data_name)

    run_model(app_data, instruction_data, args.predict, args.features)
