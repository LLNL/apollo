from rlsl.transformers import *


def get_pipeline_steps(kind=None, data=None, dropped_features=None):
    if not kind:
        kind = 'policy'

    elif 'policy' in kind:
        return [('threads', DropThreads()),
            ('duplicates', FilterDuplicates()),
           #('merge', MergeMapper(data, column='loop')),
            ('demunge', StringifyPolicies()),
            ('shuffle', ShuffleDataframe()),
            ('y', GetLabels()),
            ('drop features', FeatureDropper(columns=dropped_features))]
    elif 'thread' in kind:
        return [('threads', SelectThreads()),
            ('duplicates', FilterDuplicates()),
            ('merge', MergeMapper(data, column='loop')),
            ('demunge', StringifyPolicies()),
            ('shuffle', ShuffleDataframe()),
            ('y', GetThreads()),
            ('drop features', FeatureDropper(columns=dropped_features))]
