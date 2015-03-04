from collections import defaultdict

import numpy as np

class PerformanceErrorMetric(object):
    """
    Class implementing weighted classifier scoring based on distance from
    optimal policy runtime.
    """
    
    def __init__(self, dataframe, column_names, label_encoder):
        """
        Create a new PerformanceErrorMetric object with the provided runtime
        data.

        :param runtimes: pandas DataFrame containing loop, size and runtime
        columns.
        """
        self._data_frame = dataframe
        self._names = column_names
        self._label_encoder = label_encoder


    def score(self, estimator, X, y):
        """
        Return score based on distance from optimal policy runtime.
        """

        ei = defaultdict(list)

        for row in X:
            y = self._label_encoder.classes_[estimator.predict(row)][0].values()[0]

            loop = row[self._names.index('loop')]
            size = row[self._names.index('problem size')]

            best_time = self._data_frame[(self._data_frame['loop'] == loop) &
                    (self._data_frame['problem size'] == size)]['time_best'].values[0]

            time = self._data_frame[(self._data_frame['loop'] == loop) &
                    (self._data_frame['problem size'] == size) &
                    (self._data_frame['inner'] == y.split(' ')[0]) &
                    (self._data_frame['outer'] == y.split(' ')[1])]['time'].values[0]

            ei[size].append((time-best_time)/best_time)
            oi[size].append(best_time)

            norm = sklearn.preprocessing.StandardScaler(copy=False)
            norm.fit_transform(ei[size])

        E = 0.0
        for size in np.unique(self._data_frame['problem size'].values):
            total = 0.0

            for loop in np.unique(self._data_frame['loop'].values):
                best_time_frame = self._data_frame[(self._data_frame['loop'] == loop) &
                        (self._data_frame['problem size'] == size)]['time_best']

                if len(best_time_frame.index) > 0:
                    total = total + best_time_frame.values[0]

            for e,o in zip(ei[size], oi[size]):
                E = E + (o/total)*e

        print E
        return 1.0-E
