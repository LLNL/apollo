import numpy as np

def get_times(data, size, policy):
    return data[np.logical_and(
        data['problem size'] == size,
        np.logical_and(
            data['inner'] == policy['inner'],
            data['outer'] == policy['outer']))]['time']

def get_best_time(data, size):
    sdata = data[data['problem size'] == size]
    return [np.min(sdata[sdata['loop'] == loop]['time']) for loop in np.unique(sdata['loop'])]

def get_best_label(data, size):
    sdata = data[data['problem size'] == size]
    best_labels = []

    for loop in np.unique(sdata['loop']):
        ldata = sdata[sdata['loop'] == loop]
        best_labels.append(ldata[ldata['time'].argmin()][['inner', 'outer']])

    return best_labels
