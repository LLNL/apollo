#!/usr/bin/env python3

import argparse
import subprocess
import os

# TODO: compare between local vs. collective training

def main():
    parser = argparse.ArgumentParser(description='Run an exhaustive experiments for a program.')
    parser.add_argument('-n', '--npolicies', type=int, help='the number of policies to test.', required=True)
    parser.add_argument('-x', '--exe', help='the executable to run with arguments.', required=True)
    args = parser.parse_args()

    print('args.npolicies ', args.npolicies)

    subenv = os.environ.copy()
    subenv['APOLLO_TRACE_CSV'] = '1'
    subenv['APOLLO_RETRAIN_ENABLE'] = '0'
    for i in range(0, args.npolicies):
        subenv['APOLLO_INIT_MODEL'] = 'Static,%d'%(i)
        subprocess.run(args.exe, shell=True, check=True, env=subenv)

    #subenv['APOLLO_COLLECTIVE_TRAINING'] = '0'
    #subenv['APOLLO_LOCAL_TRAINING'] = '1'
    subenv['APOLLO_INIT_MODEL'] = 'RoundRobin'
    subprocess.run(args.exe, shell=True, check=True, env=subenv)

if __name__ == "__main__":
    main()
