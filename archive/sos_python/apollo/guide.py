#!/usr/bin/env python3


import os
import sys
import json
import time
import io
import warnings
import numpy   as np
import pandas  as pd
import sklearn as skl
from sklearn.preprocessing   import StandardScaler
from sklearn.tree            import DecisionTreeClassifier
from sklearn.tree            import DecisionTreeRegressor
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.svm             import SVC

from ssos import SSOS

from apollo.debug import log
from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT

import apollo.utils


def analyzePerformance(log, data):
    package_of_models = {}
    return package_of_models
