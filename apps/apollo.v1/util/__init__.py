import os
import datetime

import numpy as np

def df_to_unique_csv(df, file_prefix):
    now = datetime.datetime.now()
    d = '%s-%s-%s' % (now.year, now.month, now.day)

    if not os.path.exists(d):
        os.makedirs(d)

    i = 0
    while os.path.exists("%s/%s.%s.csv" % (d, file_prefix, i)):
        i += 1

    df.to_csv(path_or_buf='%s/%s.%s.csv' % (d, file_prefix, i), index=False)


def get_train_test_inds(y,train_proportion=0.7):
    '''Generates indices, making random stratified split into training set and testing sets
    with proportions train_proportion and (1-train_proportion) of initial sample.
    y is any iterable indicating classes of each observation in the sample.
    Initial proportions of classes inside training and
    testing sets are preserved (stratified sampling).
    '''

    y=np.array(y)
    train_inds = np.zeros(len(y),dtype=bool)
    test_inds = np.zeros(len(y),dtype=bool)
    values = np.unique(y)
    for value in values:
        value_inds = np.nonzero(y==value)[0]
        np.random.shuffle(value_inds)
        n = int(train_proportion*len(value_inds))

        train_inds[value_inds[:n]]=True
        test_inds[value_inds[n:]]=True

    return train_inds,test_inds
