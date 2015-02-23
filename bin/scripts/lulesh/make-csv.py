#!/usr/local/bin/python2.7.3

import re
from collections import defaultdict

import sys
sys.path.append("/g/g92/ukbeck/Projects/Machine/src/scripts")
import translate

from subprocess import *

#segit_policies = ['SEGIT_SEQ', 'SEGIT_OMP']
#seg_policies = ['SEG_SEQ', 'SEG_SIMD', 'SEG_OMP']
#sizes = [5, 10, 15, 20, 25, 30, 50, 75, 100]
segit_policies = ['SEGIT_SEQ']
seg_policies = ['SEG_OMP']
sizes = [30]

if sys.argv[1]:
    data = str(sys.argv[1])
else:
    data = 'data.csv'

with open(data, 'w') as data_file:
  for segit in segit_policies:
    for seg in seg_policies:
      for size in sizes:
        #filename = segit + "-" + seg + "-" + str(size) + ".out"
        filename = 'times.out'
        #print filename
        with open(filename, 'r') as f:
          for line in f:
            line = line.replace(' : ', ':')

            #path = (line.split(',')[2]).split(':')[-1]
#            print '================================================================================'
#            print line.split(',')[2]
#            for path in (line.split(',')[2]).split(':'):
#              translated_path = re.sub(translate.expr, translate.translate, path)
#              print translated_path + ' !! ',
#            print '================================================================================'

            translated_paths = [re.sub(translate.expr, translate.translate, path) for path in (line.split(',')[2]).split(':')]
            no_par_paths = [path for path in translated_paths if not "_par_" in path]

            #print no_par_paths[-1]

            linenum = '??' # translated_path.split(':')[-1]
            func = no_par_paths[-1].split('=')[-1]  #translated_path.split('\t')[0]
            args = ['c++filt', '-p', '%s' % func]
            proc = Popen(args, stdout=PIPE, stdin=PIPE)
            filt_func = proc.stdout.readline().rstrip('\n')

            path = (line.split(',')[2])
            line = line.replace(path, '%s' % (func, ))

            line = line.replace(': ', ',')
            line = ''.join(line.split())
            labels = str(size) + ',' + segit + ',' + seg
            data_file.write(labels + ',' + line + '\n')
