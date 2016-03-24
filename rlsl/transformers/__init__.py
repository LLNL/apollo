import pandas as pd
import numpy as np

import logging

import re

from sklearn.base import TransformerMixin, BaseEstimator
from sklearn_pandas import DataFrameMapper
from sklearn.preprocessing import LabelEncoder, OneHotEncoder, StandardScaler


class DataframePipeline():
    def __init__(self, steps):
        self.names, self.transformers = zip(*steps)
        logging.info('Creating DataframePipeline with steps: [%s]', ', '.join(s for s in self.names))
        self.steps = list(steps)
        self.X = {}

    def fit_transform(self, X):
        Xt = X
        y = None

        for name, transform in self.steps:
            logging.info('Applying %s', name)
            self.X[name] = Xt
            if 'y' == name:
                y = transform.transform(Xt)
            else:
                Xt = transform.transform(Xt)

        return Xt, y

    def __getitem__(self, key):
        return self.transformers[self.names.index(key)]

    def get_x(self, name):
        return self.X[name]


class MergeMapper():
    def __init__(self, data, column='loop'):
        self.column = column
        self.data = data

    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        df = pd.merge(X, self.data, left_on='loop_id', right_on=self.column,
                        how='left').dropna()
        logging.debug('Dropped %d records due to NaN in MergeMapper.' % (len(X) - len(df)))
        return df


class AutoDataFrameMapper(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        # grab column names of numerical features
        numerical_features = list(X.select_dtypes(include=['number']))
        # grab column names of categorical features
        categorical_features = list(X.select_dtypes(exclude=['number']))

        self.feature_names_ = numerical_features + categorical_features

        self.encoders_ = {}

        # build up list of (column, prepprocessing) tuples
        # - numerical columns are scaled, string columns are encoded
        #args = [([x for x in numerical_features], StandardScaler())]
        args = [([x for x in numerical_features], None)]
        for x in categorical_features:
            self.encoders_[x] = LabelEncoder()
            args.append(([x], self.encoders_[x]))

        return DataFrameMapper(args).fit_transform(X)

    def get_feature_list(self, X):
        numerical_features = list(X.select_dtypes(include=['number']))
        categorical_features = list(X.select_dtypes(exclude=['number']))
        return numerical_features + categorical_features

    def get_label_encoder(self, column):
        """
        Return a LabelEncoder for the named column.
        """
        return self.encoders_[column]


class DuplicateDropper(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        columns = transform_params.get('id_column', 'loop_id')
        return X.drop_duplicates(subset=[columns, 'problem_size'])


class FeatureDropper(TransformerMixin):
    def __init__(self, columns=None):
        self.cols = columns

    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        columns = transform_params.get('columns', None)
        if self.cols:
            columns = self.cols

        droppers = [x for x in self.cols if x in X]
        return X.drop(droppers, axis=1)


class ColumnSelector(TransformerMixin, BaseEstimator):
    def __init__(self, columns=None):
        self.columns = columns

    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        return X[self.columns]


class AddBestTime(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        df['exec_it_best'] = df.apply(lambda row: str(row['seg_it_best'] + ' ' + row['seg_exec_best']), axis=1)


class GetFraction(object):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        self.le = LabelEncoder()
        return self.le.fit_transform(X.dynamic_fraction)

    def get_labels(self):
        return self.le.classes_

    def get_encoder(self):
        return self.le


class GetLabels(object):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        #X['exec_it_best'] = X.apply(lambda row: str(row['seg_it_best'] + ' ' + row['seg_exec_best']), axis=1)
        #return DataFrameMapper([(['exec_it_best'], LabelEncoder())]).fit_transform(X.apply(lambda row: str(row['seg_it_best'] + ' ' + row['seg_exec_best']), axis=1))
        self.le = LabelEncoder()
        #return self.le.fit_transform(X.apply(lambda row: str(row['seg_it'] + ' ' + row['seg_exec']), axis=1))
        return self.le.fit_transform(X.policy)

    def get_labels(self):
        return self.le.classes_

    def get_encoder(self):
        return self.le

class GetChunks(object):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        self.le = LabelEncoder()
        return self.le.fit_transform(X.chunk_size)

    def get_labels(self):
        return self.le.classes_

    def get_encoder(self):
        return self.le


class GetThreads(object):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        self.le = LabelEncoder()
        return self.le.fit_transform(X['num_threads'])

    def get_labels(self):
        return self.le.classes_

    def get_encoder(self):
        return self.le


class GetTimes(object):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        return X['time.duration'].astype(float)


class DropThreads(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        tcol = transform_params.get('thread_column', 'num_threads')
        if tcol in X:
            return X[((X[tcol] == 1) & ((X.seg_exec != 'SEG_OMP') & (X.seg_it != 'SEGIT_OMP'))) |
                    ((X[tcol] == 16) & ((X.seg_exec == 'SEG_OMP') | (X.seg_it == 'SEGIT_OMP')))]
        else:
            return X

class SelectThreads(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        return X[(X['seg_exec'] == 'SEG_OMP')
                | (X['seg_it'] == 'SEGIT_OMP')]


class FilterDuplicates(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        #return X.loc[X.groupby(['numeric_loop_id', 'problem_size'])
            #['time.duration'].idxmin()]
        #return X.sort('time.inclusive.duration').groupby(['problem_name', 'numeric_loop_id', 'problem_size'], as_index=False).first()
        return X.sort('time.inclusive.duration').groupby(['numeric_loop_id', 'problem_size'], as_index=False).first()


class ShuffleDataframe(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        return X.iloc[np.random.permutation(len(X))]


class ReorderCols(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        cols = list(X)
        cols.insert(0, cols.pop(cols.index('numeric_loop_id')))
        cols.insert(0, cols.pop(cols.index('problem_size')))
        return X[cols]


class SplitPolicies():
    def demunge_name(self, name):
        policy_regex = re.compile('[a-z]+_[a-z]+')
        return policy_regex.search(name).group(0).upper()

    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        X['seg_it'] = X.apply(lambda row: self.demunge_name(str(row['policy'])), axis=1)
        X['seg_exec'] = X.apply(lambda row: self.demunge_name(str(row['seg_exec'])), axis=1)

        return X

class DropPolicies(TransformerMixin):
    def __init__(self, policy):
        self.policy = policy

    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        return X[~((X.seg_it.str.contains(self.policy, na=False)) |
                 (X.seg_exec.str.contains(self.policy, na=False)))]


class MakeFraction():
    def __init__(self, instructions):
        self.instructions = instructions

    def transform(self, X):
        for col in self.instructions:
            #print col
            if col is 'loop':
                continue
            X[col] = X[col].astype(float)/X['func_size']

        return X


