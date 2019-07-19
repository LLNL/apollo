#!/usr/bin/env python
import os
import sys
import json
import time
import cStringIO
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

from config import VERBOSE
from config import DEBUG
from config import FRAME_INTERVAL



def tablePrint(results):
    # Print out the results in a pretty column-aligned way:
    widths = [max(map(len, str(col))) for col in zip(*results)]
    for row in results:
        print "    ",
        print "  ".join((val.ljust(width) for val, width in zip(row, widths)))
    #
    return


def generateRoundRobinModel(SOS, data, region_names):
    model_def = {
        "type": {
            "guid": SOS.get_guid(),
            "name": "RoundRobin",
        },
        "region_names": list(region_names),
        "features": {
            "count": 0,
            "names": "NA",
        },
        "driver": {
            "format": "int",
            "rules": 1,
        }
    }



def generateRandomModel(SOS, data, region_names):
    model_def = {
        "type": {
            "guid": SOS.get_guid(),
            "name": "Random",
        },
        "region_names": list(region_names),
        "features": {
            "count": 0,
            "names": "NA",
        },
        "driver": {
            "format": "int",
            "rules": 1,
        }
    }

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4,
            ensure_ascii=True) + "\n"
    return model_as_json

def generateStaticModel(SOS, data, region_names):
    model_def = {
        "type": {
            "guid": SOS.get_guid(),
            "name": "Static",
        },
        "region_names": list(region_names),
        "features": {
            "count": 0,
            "names": "NA",
        },
        "driver": {
            "format": "int",
            "rules": 1,
        }
    }

    model_as_json = json.dumps(model_def, sort_keys=False, indent=4,
            ensure_ascii=True) + "\n"
    return model_as_json

def progressBar(amount, total, length, fill='='):
    if amount >= total:
        return fill * length
    if length < 4: length = 4
    fillLen = int(length * amount // total)
    emptyLen = length - 1 - fillLen
    bar = (fill * fillLen) + ">" + ("-" * emptyLen)
    return bar


