class PerformanceErrorMetric(object):
    """
    Class implementing weighted classifier scoring based on distance from
    optimal policy runtime.
    """
    
    def __init__(self, dataframe, x_data_mapper, y_data_mapper):
        """
        Create a new PerformanceErrorMetric object with the provided runtime
        data.

        :param runtimes: pandas DataFrame containing loop, size and runtime
        columns.
        """
        self._data_frame = dataframe

    def score(self, estimator, X, y):
        """
        Return score based on distance from optimal policy runtime.
        """

        print X
        print y

        return 0.0
