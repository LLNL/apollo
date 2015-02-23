#!/usr/local/bin/python2.7.3

import re
from collections import defaultdict

import sys
sys.path.append("/g/g92/ukbeck/Projects/Machine/src/scripts")
import translate

from subprocess import *

import argparse

segit_policies = ['SEGIT_SEQ', 'SEGIT_OMP']
seg_policies = ['SEG_SEQ', 'SEG_SIMD', 'SEG_OMP']
sizes = [1, 2, 3]

write_paths = False
write_data = True

data = 'data.csv'

parser = argparse.ArgumentParser()
parser.add_argument('-p', '--write-paths', action='store_true')
parser.add_argument('-C', action='store_const', const=1)
args = parser.parse_args()

print vars(args)

write_paths = args.write_paths

if write_paths:
    for segit in segit_policies:
        for seg in seg_policies:
            for size in sizes:
                filename = segit + "-" + seg + "-" + str(size) + ".out"
                with open(filename, 'r') as f:
                    for line in f:
                        if not 'ares-' in line:
                            continue

                        line = line.replace(' : ', ':')

                        translated_paths = [re.sub(translate.expr, translate.translate, path) for path in (line.split(',')[2]).split(':')]
                        no_par_paths = [path for path in translated_paths if not "_par_" in path]

                        for path in translated_paths:
                            sys.stdout.write(path.split('=')[0] + '!!')
                        sys.stdout.write('\n')
else:
    with open(data, 'w') as data_file:
        for segit in segit_policies:
            for seg in seg_policies:
                for size in sizes:
                    filename = segit + "-" + seg + "-" + str(size) + ".out"
                    with open(filename, 'r') as f:
                        for line in f:
                            if not 'ares-' in line:
                                continue
                            if "Symtab" in line:
                                continue

                            line = line.replace(' : ', ':')

                            translated_paths = [re.sub(translate.expr, translate.translate, path) for path in (line.split(',')[2]).split(':')]
                            no_par_paths = [path for path in translated_paths if not "_par_" in path]

                            if write_data:
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
