import argparse
import warnings

import numpy as np
import pandas as pd
from IPython import embed

from sklearn.tree import DecisionTreeClassifier
from sklearn.pipeline import Pipeline
from sklearn.cross_validation import cross_val_score
from sklearn.decomposition import PCA

import apollo
from apollo.pipeline import get_pipeline_steps
from apollo.transformers import DataframePipeline, AutoDataFrameMapper

from apollo.util.loader import PandasCaliperLoader, PandasInstructionLoader
from apollo.codegen import CodeGenerator
from apollo.util.timer import Timer

description = "Interactive session just loads up the files"

def setup_parser(subparser):
    subparser.add_argument(
        'files', nargs=argparse.REMAINDER, help="files containing application samples and instruction data")

def interactive(parser, args):
    warnings.simplefilter("ignore")

    if not args.files:
        sys.stderr.write("install requires two files of application samples\n")
        sys.exit(-1)

    app_data_name, instruction_data_name = (args.files[0], args.files[1])
    app_data, instruction_data = apollo.util.loader.load(app_data_name, instruction_data_name)

    embed()
