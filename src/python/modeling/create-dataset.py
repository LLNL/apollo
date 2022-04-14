#!/usr/bin/env python3

import argparse
import pandas as pd
import glob

'''
Reads data from CSV traces and creates a loadable dataset for training models.
'''


def print_to_str(index, features, policy, xtime):
    features = [str(n) for n in features]
    output = '  ' + str(index) + ': { features: [ '
    for fi in features:
        output += fi + ', '
    output += ']'
    output += ', policy: ' + str(policy)
    output += ', xtime: ' + str(xtime)
    output += ' },\n'
    return output


def agg_by_none(data_per_region):
    output = '# agg by none\n'

    output += 'data: {\n'
    feature_cols = list(data_per_region.columns[5:-2])

    # Output all the data points.
    for i, row in data_per_region.iterrows():
        output += print_to_str(i, row[feature_cols],
                               row['policy'], row['xtime'])

    output += '}\n\n'
    return output


def agg_by_mean(data_per_region):
    output = '# agg by mean\n'

    output += 'data: {\n'
    feature_cols = list(data_per_region.columns[5:-2])

    # Group by features and policy to extract policies per feature with
    # their mean xtime.
    groups = data_per_region.groupby(by=feature_cols + ['policy'])
    index = 0
    for name, g in groups:
        features = name[:-1]
        policy = name[-1]
        xtime = g['xtime'].mean()
        output += print_to_str(index, features, policy, xtime)
        index += 1

    output += '}\n\n'
    return output


def agg_by_min(data_per_region):
    output = '# agg by min\n'

    output += 'data: {\n'
    feature_cols = list(data_per_region.columns[5:-2])

    groups = data_per_region.groupby(by=feature_cols)
    index = 0
    for _, g in groups:
        # Find the index of the minimum xtime.
        min_g = g.loc[g['xtime'].idxmin()]

        # Extract data from the minimum xtime data point.
        features = min_g[feature_cols]
        policy = min_g['policy']
        xtime = min_g['xtime']
        output += print_to_str(index, features, policy, xtime)
        index += 1

    output += '}\n\n'
    return output


def agg_by_mean_min(data_per_region):
    output = '# agg by mean_min\n'

    output += 'data: {\n'
    feature_cols = list(data_per_region.columns[5:-2])

    groups = data_per_region.groupby(by=feature_cols)
    index = 0
    for name, g in groups:
        # Group by policy to compute the mean xtime.
        g2 = g.groupby(by=['policy'])
        mean_g = g2['xtime'].mean()

        # Find the index (equals the policy) with the minimum mean xtime.
        idxmin = mean_g.idxmin()

        # Create a list of feature values if there is a single feature.
        if len(feature_cols) > 1:
            features = name
        else:
            features = [name]
        policy = idxmin
        xtime = mean_g.loc[idxmin]
        output += print_to_str(index, features, policy, xtime)
        index += 1

    output += '}\n\n'
    return output


def read_tracefiles(tracefiles):
    data = pd.DataFrame()

    for f in tracefiles:
        print('Read', f)
        try:
            csv = pd.read_csv(f, sep=' ', header=0, index_col=False)
        except pd.errors.EmptyDataError:
            print('Warning: no data in', f)

        data = pd.concat([data, csv], ignore_index=True, sort=False)

    return data


def read_tracedirs(tracedirs):
    data = pd.DataFrame()

    for dir in tracedirs:
        globlist = glob.glob('%s/trace*.csv' % (dir))
        data_dir = read_tracefiles(globlist)
        data = pd.concat([data, data_dir], ignore_index=True, sort=False)

    return data


def main():
    parser = argparse.ArgumentParser(
        description='Create loadable training datasets from existing CSV measurements.')
    parser.add_argument('--tracedirs', nargs='+',
                        help='trace directories')
    parser.add_argument('--tracefiles', nargs='+',
                        help='trace filenames')
    parser.add_argument('--agg',
                        help='aggregate measures ', choices=['none', 'mean', 'min', 'mean-min'], required=True)
    args = parser.parse_args()

    if not (args.tracedirs or args.tracefiles):
        raise RuntimeError('Either tracedirs or tracefiles must be set')

    if args.tracefiles:
        data = read_tracefiles(args.tracefiles)

    if args.tracedirs:
        data = read_tracedirs(args.tracedirs)

    #print('data\n', data)
    regions = data['region'].unique().tolist()

    # Create datasets per region.
    for r in regions:
        data_per_region = data.loc[data['region'] ==
                                   r].dropna(axis='columns').reset_index()

        if args.agg == 'none':
            output = agg_by_none(data_per_region)
        elif args.agg == 'mean':
            output = agg_by_mean(data_per_region)
        elif args.agg == 'min':
            output = agg_by_min(data_per_region)
        elif args.agg == 'mean-min':
            output = agg_by_mean_min(data_per_region)
        else:
            raise RuntimeError('Invalid aggregation args ' + str(args.agg))
        with open('Dataset-' + r + '.yaml', 'w') as f:
            f.write(output)


if __name__ == "__main__":
    main()
