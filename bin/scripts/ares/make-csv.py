#!/usr/local/bin/python2.7.3

import re
from collections import defaultdict
from itertools import product

segit_policies = ['SEGIT_SEQ', 'SEGIT_OMP', 'SEGIT_CILK']
seg_policies = ['SEG_SEQ', 'SEG_OMP', 'SEG_CILK']
sizes = [1, 2, 3, 4, 5, 6, 7, 8]

loops = defaultdict(dict)

data = 'data.csv'

with open(data, 'w') as data_file:
  for segit, seg, size in itertools.product(segit_policies, seg_policies, sizes):
    filename = segit + "-" + seg + "-" + str(size) + ".out"
    #print filename
    with open(filename, 'r') as f:
      for line in f:
        if 'Sym' in line:
            continue
        if not 'RAJA' in line:
              continue
        if 'INSTRUCTION' in line:
            break
        line = line.replace(' : ', '+')
        line = line.replace(': ', ',')
        line = line.replace('+', ':')
        line = ''.join(line.split())
        labels = str(size) + ',' + segit + ',' + seg
        #func = '@'.join(line.split(",")[2:4])
        time = line.split(",")[-1]
        #if func in loops:
        #  if loops[func]["time"] > time:
        #    loops[func]["time"] = time
        #    loops[func]["labels"] = labels
        #else:
        #  loops[func]["time"] = time
        #  loops[func]["labels"] = labels
        data_file.write(labels + ',' + line + '\n')

#print loops

#features_file = 'X.csv'
#classes_file = 'y.csv'
#times_file = 'times.csv'
#
#with open(features_file, 'w') as features, open(classes_file, 'w') as classes, open(times_file, 'w') as times:
#  with open(data, 'r') as data_file:
#    for line in data_file:
#      func = '@'.join(line.split(",")[5:7])
#      labels = ','.join(line.split(",")[:3])
#      times.write(','.join([func, loops[func]["time"] + '\n']))
#      if loops[func]["labels"] == labels:
#        classes.write(labels + '\n')
#        features.write(','.join(line.split(',')[4:-1]) + '\n')
