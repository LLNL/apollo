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
