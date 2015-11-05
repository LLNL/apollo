import logging

timers = {}
data = {}
scores = {}


dropped_features = ['loop', 'loop_id', 'callpath.address', 'num_threads', 'time.duration', 'seg_exec', 'seg_it', 'loop_count', 'reduction']

