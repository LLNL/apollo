#!/usr/bin/env python
import os
import sys
import json
import time
import io
import warnings

from ssos import SSOS

from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT

import apollo.utils

def log(level, msg):
    if (level <= VERBOSE):
        indent = "    " * level
        print("== CONTROLLER: " + indent + msg)
    return


