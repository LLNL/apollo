#!/usr/bin/env python3

import argparse
import pandas as pd
import time
import numpy as np
import glob

# TODO: merge data per rank to analyze (clarity, performance)
# TODO: histogram for the distribution of policies
# TODO: retrofit the best policy selection to Apollo
# TODO: separate the application overhead with the Apollo overhead: run non-apollo version vs. apollo version 
# with Static,0 policy

def main():
    parser = argparse.ArgumentParser(description='Analyze exhaustive experiments for a program.')
    parser.add_argument('-n', '--npolicies', type=int, help='the number of policies to test.', required=True)
    parser.add_argument('-np', '--nprocesses', type=int, help='the number of processes (ranks).', required=True)
    parser.add_argument('-d', '--dir', help='the directory that contains the trace files.', required=True)
    args = parser.parse_args()

    # Outer index rank, inner index static policy
    print('=== Reading csv files...')
    data_map= {}
    globlist = glob.glob('%s/trace*.csv'%(args.dir))
    nfiles = len(globlist)
    for i, f in enumerate(globlist):
        print('\r=== Reading %-64s %06d/%06d'%(f, i+1, nfiles), flush=True, end='')
        data_map[f] = pd.read_csv(f, sep= ' ', header=0)
    print()

    data = pd.concat(data_map.values(), ignore_index=True)

    for i in range(args.nprocesses):
        # Print execution time for Static training.
        for j in range(args.npolicies):
            xtimes = data.loc[ \
                    (data['rankid']==i) & \
                    (data['training']=='Static,%d'%(j)) \
                    ]['xtime']
            print('Rank %d Static,%d xtime %.6f'%(i, j, sum(xtimes)))
        # Print execution time for RoundRobin
        xtimes = data.loc[ \
                (data['rankid']==i) & \
                (data['training']=='RoundRobin') \
                ]['xtime']
        print('Rank %d RoundRobin xtime %.6f'%(i, sum(xtimes)))

    print()
    print('=== Computing optimal policy selection...')
    for i in range(args.nprocesses):
        t1 = time.perf_counter()
        # Find optimal policy selection (fastest way): select by rank equals i and training equals to Static;
        # sort by xtime; drop_duplicates grouped by region, idx to keep minimum;
        # sort by index to return in original order.
        opt = data.loc[(data['rankid']==i) & (data['training'].str.contains('Static'))].\
                sort_values(by=['xtime']).drop_duplicates(['region', 'idx'], keep='first').\
                sort_index()
        t2 = time.perf_counter()
        #print('=========== OPT =============\n', opt, '=============================')
        print('opt-rank-%d eval time %.6f s'%(i, t2-t1))
        print('opt-rank-%d xtime %.6f'%(i, sum(opt['xtime'])))
        print('=== Write optimal selection CSV for rank %d'%(i))
        opt.to_csv('opt-rank-%d.csv'%(i), sep=' ', index=False)

        print('=== Compute RoundRobin accuracy...')
        # Create dataframes indexed by region, idx to match with isin() in opt (avoids index mismatch).
        data_rr_region_idx = data.loc[(data['rankid']==i) & (data['training']=='RoundRobin')]\
            [['region', 'idx', 'policy']].set_index(['region','idx'])
        opt_region_idx = opt[['region','idx','policy']].set_index(['region', 'idx'])
        cond = data_rr_region_idx.isin(opt_region_idx)

        n_rr_opt = cond.all(axis=1).value_counts()[True]
        nrows = len(cond.index)
        print('Rank %d accuracy %.2f%%'%(i, (n_rr_opt*100.0)/nrows))

        (hist, bins) = np.histogram(data_rr_region_idx['policy'], bins=np.arange(args.npolicies+1))
        print('Rank %d'%(i), 'bins', bins[:-1], 'hist', hist, '\n')

if __name__ == "__main__":
    main()
