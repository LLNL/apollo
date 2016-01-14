import logging

timers = {}
data = {}
scores = {}


dropped_features = ['loop', 'loop_id', 'callpath.address', 'num_threads', 'time.phase.duration', 'time.duration', 'policy', 'loop_count', 'reduction', 'timer', 'numeric_loop_id']

chunk_dropped_features = ['loop', 'loop_id', 'callpath.address', 'num_threads', 'time.duration', 'policy', 'loop_count', 'reduction', 'timer', 'numeric_loop_id', 'chunk_size']
