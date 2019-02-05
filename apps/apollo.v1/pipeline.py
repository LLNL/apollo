from apollo.transformers import *


def get_pipeline_steps(kind=None, data=None, dropped_features=None):
    if not kind:
        kind = 'policy'

    if 'policy' in kind:
        return [
            #('threads', DropThreads()),
            ('duplicates', FilterDuplicates()),
            ('merge', MergeMapper(data, column='loop')),
            #('instruction_fracs', MakeFraction(list(data))),
            #('demunge', StringifyPolicies()),
            ('shuffle', ShuffleDataframe()),
            ('y', GetLabels()),
            ('drop features', FeatureDropper(columns=dropped_features))]
    elif 'chunk' in kind:
        return [
            ('duplicates', FilterDuplicates()),
            ('merge', MergeMapper(data, column='loop')),
            ('shuffle', ShuffleDataframe()),
            ('y', GetChunks()),
            ('drop features', FeatureDropper(columns=dropped_features))]
    elif 'thread' in kind:
        return [#('threads', SelectThreads()),
            ('duplicates', FilterDuplicates()),
            ('merge', MergeMapper(data, column='loop')),
            #('demunge', StringifyPolicies()),
            ('shuffle', ShuffleDataframe()),
            ('y', GetThreads()),
            ('drop features', FeatureDropper(columns=dropped_features))]
    elif 'dynamic' in kind:
        return [#('threads', SelectThreads()),
            ('duplicates', FilterDuplicates()),
            ('merge', MergeMapper(data, column='loop')),
            #('demunge', StringifyPolicies()),
            ('shuffle', ShuffleDataframe()),
            ('y', GetFraction()),
            ('drop features', FeatureDropper(columns=dropped_features))]
    elif 'regression' in kind:
        return [('merge', MergeMapper(data, column='loop')),
            ('shuffle', ShuffleDataframe()),
            ('y', GetTimes()),
            ('drop features', FeatureDropper(columns=dropped_features))]

