from collections import defaultdict

import numpy as np

import sklearn.preprocessing
from sklearn.feature_extraction import DictVectorizer

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
        self._O = {}

    def get_optimal_times(self):
        for size in np.unique(self._data_frame['problem size'].values):
            total = 0.0
            for loop in np.unique(self._data_frame['loop'].values):
                best_time_frame = self._data_frame[(self._data_frame['loop'] == loop) &
                        (self._data_frame['problem size'] == size)]['time_best']

                if len(best_time_frame.index) > 0:
                    total = total + best_time_frame.values[0]

            self._O[size] = total


    def get_predicted_times(self, estimator, fX, X, y):
        loops = {}
        p = {}

        for size in np.unique(self._data_frame['problem size'].values):
            loops[size] = []
            p[size] = 0.0

        for row,full_row in zip(X,fX):
            y = self._label_encoder.classes_[estimator.predict(row)][0].values()[0]

            loop = full_row[self._names.index('loop')]
            size = full_row[self._names.index('problem size')]

            if not loop in loops[size]:
                loops[size].append(loop)

                time = self._data_frame[(self._data_frame['loop'] == loop) &
                        (self._data_frame['problem size'] == size) &
                        (self._data_frame['inner'] == y.split(' ')[0]) &
                        (self._data_frame['outer'] == y.split(' ')[1])]['time']

                if len(time.index) > 0:
                    p[size] = p[size] + time.values[0]

        return p



    def score(self, estimator, fX, X, y):
        """
        Return score based on distance from optimal policy runtime.
        """

        E = []
        loops = {}
        p = {}

        self.get_optimal_times()

        for size in np.unique(self._data_frame['problem size'].values):
            loops[size] = []
            p[size] = 0.0
            total = 0.0

        for row,full_row in zip(X,fX):
            y = self._label_encoder.classes_[estimator.predict(row)][0].values()[0]

            loop = full_row[self._names.index('loop')]
            size = full_row[self._names.index('problem size')]

            if not loop in loops[size]:
                loops[size].append(loop)

                time = self._data_frame[(self._data_frame['loop'] == loop) &
                        (self._data_frame['problem size'] == size) &
                        (self._data_frame['inner'] == y.split(' ')[0]) &
                        (self._data_frame['outer'] == y.split(' ')[1])]['time']

                if len(time.index) > 0:
                    p[size] = p[size] + time.values[0]

        for size in np.unique(self._data_frame['problem size'].values):
            E.append(round((p[size]/self._O[size]) - 1.0,4))

        return np.mean(E)


class RuntimeScorer(object):
    """
    """
    def __init__(self, dataframe, column_names, label_encoder):
        """
        :param runtimes: pandas DataFrame containing loop, size and runtime
        columns.
        """
        self._data_frame = dataframe
        self._names = column_names
        self._label_encoder = label_encoder

        self._names_loop = [x for x in list(self._data_frame) if x not in ['time', 'outer', 'inner', 'outer_best', 'inner_best', 'time_best']]

        x_num_loop = self._data_frame[[x for x in list(self._names_loop) if x not in ['loop type', 'set type']]].as_matrix()

        x_num = self._data_frame[[x for x in self._names if x not in ['loop type', 'set type']]].as_matrix()
        x_cat = self._data_frame[[x for x in ['loop type', 'set type'] if x in self._names]].T.to_dict().values()

        vectorizer = DictVectorizer(sparse=False)
        vec_x_cat = vectorizer.fit_transform(x_cat)

        self.X = np.hstack((x_num, vec_x_cat))
        self.X_loop = np.hstack((x_num_loop, vec_x_cat))


    def score(self, size, estimator):
        """
        Return score based on distance from optimal policy runtime.
        """

        loops = []
        total = 0.0

        for loop, row in zip(self.X_loop,self.X):
            s = loop[self._names_loop.index('problem size')]
            if s != size:
                continue

            l = loop[self._names_loop.index('loop')]
            if l in loops:
                continue
            else:
                loops.append(l)

            y = self._label_encoder.classes_[estimator.predict(row)][0].values()[0]

            time = self._data_frame[(self._data_frame['loop'] == l) &
                    (self._data_frame['problem size'] == s) &
                    (self._data_frame['inner'] == y.split(' ')[0]) &
                    (self._data_frame['outer'] == y.split(' ')[1])]['time'].values[0]
            
            total = total + time

        return total
