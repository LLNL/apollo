#!/usr/bin/env python
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

from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT


def tablePrint(results):
    # Print out the results in a pretty column-aligned way:
    widths = [max(map(len, str(col))) for col in zip(*results)]
    for row in results:
        print("  ".join((val.ljust(width) for val, width in zip(row, widths))))
    #
    return


def progressBar(amount, total, length, fill='='):
    if amount >= total:
        return fill * length
    if length < 4: length = 4
    fillLen = int(length * amount // total)
    emptyLen = length - 1 - fillLen
    bar = (fill * fillLen) + ">" + ("-" * emptyLen)
    return bar


