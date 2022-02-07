#!/usr/bin/env python3

import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import re

# TODO: assumes a single feature indicated by f0. Generalize for more features.

def plot_bars(ax, df):
    sns.countplot(data=df, x='policy', hue='f0', ax=ax)
    # build labels
    bar_labels = []
    policies = df['policy'].unique()
    policies.sort()
    features = df['f0'].unique()
    features.sort()
    #print('policies', policies)
    #print('features', features)

    for policy in policies:
        for feature in features:
            xtimes = df.loc[df['f0'] == feature].loc[df['policy'] == policy][['f0', 'policy', 'xtime']]
            if xtimes.empty:
                bar_labels.append(0)
                continue
            values = list(xtimes['xtime'])
            #values.remove(max(values))
            #values.remove(min(values))
            mean = np.mean(values)
            #print('xtimes', xtimes, 'mean', mean)
            bar_labels.append('%.2f'%(mean*1e6))
    for i, container in enumerate(ax.containers):
        #print('container', i)
        #print('bar_labels', bar_labels[i::3])
        text_list = ax.bar_label(container, labels=bar_labels[i::3])
        for t in text_list:
            t.set_rotation(0)

def main():
    parser = argparse.ArgumentParser(description='Analyze exhaustive experiments for a program.')
    parser.add_argument('--file', help='the trace file to plot.', required=True)
    parser.add_argument('--ntraining', type=int, help='the number of training executions.', required=True)
    parser.add_argument('--drop-first', help='drop first measurement outlier.', action='store_true')
    parser.add_argument('--title', help='add to title.', nargs='?', default='')
    args = parser.parse_args()

    m = re.match('trace-(.*?)-.*', args.file)
    model_name = m.group(1)
    #print('model_name', model_name)
    with open(args.file, 'r') as f:
        df = pd.read_csv(f, sep=' ', header=0)

    fig, ax = plt.subplots(1, 2, sharey=True, figsize=(12,6) )

    start = 0
    if args.drop_first:
        start = 1
    plot_bars(ax[0], df[start:args.ntraining])
    plot_bars(ax[1], df[args.ntraining:])
    ntuned = len(df[args.ntraining:])

    ax[0].legend(title='Feature')
    ax[0].spines['top'].set_visible(False)
    ax[0].spines['right'].set_visible(False)
    ax[1].spines['top'].set_visible(False)
    ax[1].spines['right'].set_visible(False)
    ax[1].get_legend().remove()
    ax[0].set_title('Training data for the first ' + str(args.ntraining-start) + \
        ' iterations (drop-first ' + str(args.drop_first) + ')')
    ax[1].set_title('Tuning data for the next ' + str(ntuned) + ' iterations')

    suptitle = model_name + '\n' + bytes(args.title, 'utf-8').decode('unicode_escape') + '\nBar labels are mean execution times in ms\n'
    plt.suptitle(suptitle)
    'Tuning block size for Apollo RAJA daxpy on HIP forall\nFeature is vector size. Policy 0: 64 threads, 1: 256 threads, 2: 1024 threads'
    #sns.catplot(data=df[501:], x='policy', y='f0', kind='bar', ax=ax[1])
    #sns.violinplot(data=df[:501], ax=ax[0], y='policy', x='f0')
    #sns.catplot(data=df[501:], ax=ax[1], y='policy', x='f0')
    #sns.violinplot(data=df, x=df.index, y='policy')
    plt.tight_layout()
    #plt.show()
    plt.savefig(model_name + '-plot.pdf')

if __name__ == "__main__":
    main()
