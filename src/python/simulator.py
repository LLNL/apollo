#!/usr/bin/env python

# Copyright (c) 2019, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory
#
# This file is part of Apollo.
# OCEC-17-092
# All rights reserved.
#
# Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
# of many collaborators.
#
# Apollo was originally created by David Beckingsale, david@llnl.gov
#
# For details, see https://github.com/LLNL/apollo.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


import os
import sys
import time
import pickle

import pandas as pd
import matplotlib as plt

##########

def process_trace(csv, flush):

    # pre-process / group data
    # build an initial model from flush data

    # go through the trace and evaluate the model
    for row in csv.head().itertuples():
        step          = int(row.step)
        region_name   = str(row.region_name)
        policy_index  = int(row.policy_index)
        num_threads   = int(row.num_threads)
        num_elements  = int(row.num_elements)
        time_exec     = float(row.time_exec)

        print("region_name: %s num_elements: %d" % (region_name, num_elements))
        if (row.Index > 10):
            break


    return

##########

def main():
    simulator_start = time.time()

    model_def = ""
    model_len = 0

    # DECISIONTREE
    #model_def = trees.generateDecisionTree(SOS, data, region_names)
    #model_len = len(model_def)

    if len(sys.argv) < 3:
        print("USAGE: ./simulator.py <apollo_trace> <apollo_flush>\n")
        return

    print("Arguments:\n%s\n" % str(sys.argv))

    print("Loading trace into Pandas...)")
    csv = pd.read_csv(str(sys.argv[1]))
    print("  OK!\n")

    print("Loading flush (training) data into Pandas...")
    flush = pd.read_csv(str(sys.argv[2]))
    print("  OK!\n")

    process_trace(csv, flush)

    return
#########



if __name__ == "__main__":
    main()
