#!/usr/bin/env python3

import argparse
import pandas as pd
import time
import numpy as np

# TODO: merge data per rank to analyze (clarity, performance)
# TODO: histogram for the distribution of policies
# TODO: retrofit the best policy selection to Apollo
# TODO: separate the application overhead with the Apollo overhead: run non-apollo version vs. apollo version 
# with Static,0 policy

def main():
    parser = argparse.ArgumentParser(description='Analyze exhaustive experiments for a program.')
    parser.add_argument('-n', '--npolicies', type=int, help='the number of policies to test.', required=True)
    parser.add_argument('-np', '--nprocesses', type=int, help='the number of processes (ranks).', required=True)
    args = parser.parse_args()

    # Outer index rank, inner index static policy
    print('=== Reading csv files...')
    data_static_per_rank = [ [ pd.read_csv('trace-Static,%d-rank-%d.csv'%(i,j), sep=' ', header=0) \
                            for i in range(args.npolicies)] \
                            for j in range(args.nprocesses)]
    
    # Index rank
    data_rr_per_rank = [ pd.read_csv('trace-RoundRobin-rank-%d.csv'%(i), sep=' ', header=0) \
                        for i in range(args.nprocesses)]

    nrows = len(data_static_per_rank[0][0].index)
    for i in range(args.nprocesses):
        for j,data in enumerate(data_static_per_rank[i][1:], 1):
            assert(nrows == len(data.index)),'nrows(%d) != data[rank=%d][static=%d]'%(nrows,i,j)
        assert(nrows == len(data_rr_per_rank[i].index)),'nrows(%d) != data_rr_per_rank[rank=%d]'%(nrows,i)


    for i in range(args.nprocesses):
        for j, data in enumerate(data_static_per_rank[i]):
            print('Rank %d Static,%d xtime %.6f'%(i,j,sum(data['xtime'])))
        print()

    for i in range(args.nprocesses):
        print('Rank %d RoundRobin xtime %.6f'%(i,sum(data_rr_per_rank[i]['xtime'])))
    print()

    print('=== Computing optimal policy selection...')
    for i in range(args.nprocesses):
        t1 = time.perf_counter()
        # Find optimal policy selection: concat, groupby region and index, keep only the ones with least xtime
        opt = pd.concat(data_static_per_rank[i]).groupby(by=['region', 'idx']).\
            apply(lambda group: group.nsmallest(1, columns='xtime'))\
            .reset_index(drop=True)
        t2 = time.perf_counter()
        opt.to_csv('opt-rank-%d.csv'%(i), sep=' ', index=False)
        #print('=========== OPT =============', opt, '=============================')
        print('opt-rank-%d eval time %.6f s'%(i,t2-t1))
        print('opt-rank-%d xtime %.6f'%(i,sum(opt['xtime'])))

        # Find rows in data_rr AND in opt, so those are optimal selections
        cond = data_rr_per_rank[i].drop('xtime', axis=1).isin(opt.drop('xtime', axis=1))
        n_rr_opt = cond.all(axis=1).value_counts()[True]
        #rr_not_opt = data_rr.drop(data_rr[cond.all(axis=1)].index)
        print('Rank %d accuracy %.2f%%'%(i, (n_rr_opt*100.0)/nrows))

        (hist, bins) = np.histogram(data_rr_per_rank[i]['policy'], bins=np.arange(args.npolicies+1))
        print('Rank %d'%(i), 'bins', bins, 'hist', hist, '\n')
        # Verify
        #for i in range(0, args.npolicies):
        #    print(i, data_rr['policy'].value_counts()[i])


if __name__ == "__main__":
    main()