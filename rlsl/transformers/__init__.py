import pandas as pd

from sklearn.base import TransformerMixin, BaseEstimator
from sklearn_pandas import DataFrameMapper
import pdb; pdb.set_trace()  # XXX BREAKPOINT
from sklearn.preprocessing import LabelEncoder, OneHotEncoder, StandardScaler

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
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        columns = transform_params.get('columns', None)
        return X.drop(columns, axis=1)


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


class GetLabels(object):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        #X['exec_it_best'] = X.apply(lambda row: str(row['seg_it_best'] + ' ' + row['seg_exec_best']), axis=1)
        #return DataFrameMapper([(['exec_it_best'], LabelEncoder())]).fit_transform(X.apply(lambda row: str(row['seg_it_best'] + ' ' + row['seg_exec_best']), axis=1))
        self.le = LabelEncoder()
        return self.le.fit_transform(X.apply(lambda row: str(row['seg_it'] + ' ' + row['seg_exec']), axis=1))

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
        return X['time.duration_best']


class DropThreads(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        tcol = transform_params.get('thread_column', 'num_threads')
        return X[((X[tcol] == 1) & ((X.seg_exec != 'SEG_OMP') & (X.seg_it != 'SEGIT_OMP'))) |
                 ((X[tcol] == 16) & ((X.seg_exec == 'SEG_OMP') | (X.seg_it == 'SEGIT_OMP')))]

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
        return X.loc[X.groupby(
            ['loop_count', 'problem_size'])
            ['time.duration'].idxmin()]


class ShuffleDataframe(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        return X.iloc[np.random.permutation(len(df))]

class ReorderCols(TransformerMixin):
    def fit(self, X, y=None, **fit_params):
        return self

    def transform(self, X, **transform_params):
        cols = list(X)
        cols.insert(0, cols.pop(cols.index('loop_count')))
        cols.insert(0, cols.pop(cols.index('problem_size')))
        return X[cols]

