import logging

timers = {}
data = {}
scores = {}

#dropped_features = ['loop', 'loop_id', 'callpath.address', 'num_threads', 'time.phase.duration', 'time.duration', 'policy', 'loop_count', 'reduction', 'timer', 'numeric_loop_id']

lulesh_chunk_dropped_features = ['cali.caliper.version', 'cali.snapshot.event.attr.level', 'cali.snapshot.event.end', 'callpath.address', 'chunk_size', 'loop_id', 'numeric_loop_id', 'policy', 'time.inclusive.duration', 'timer', 'loop']
lulesh_policy_dropped_features = ['cali.caliper.version', 'cali.snapshot.event.attr.level', 'cali.snapshot.event.end', 'callpath.address', 'loop_id', 'numeric_loop_id', 'policy', 'time.inclusive.duration', 'timer', 'loop']
lulesh_dynamic_dropped_features = ['cali.caliper.version', 'cali.snapshot.event.attr.level', 'cali.snapshot.event.end', 'callpath.address', 'loop_id', 'dynamic_fraction', 'numeric_loop_id', 'policy', 'time.inclusive.duration', 'timer', 'loop']

regression_dropped_features = ['loop', 'loop_id', 'callpath.address', 'num_threads', 'time.phase.duration', 'time.duration', 'loop_count', 'reduction', 'timer', 'numeric_loop_id']
chunk_dropped_features = ['loop', 'loop_id', 'callpath.address', 'num_threads', 'time.duration', 'policy', 'loop_count', 'reduction', 'timer', 'numeric_loop_id', 'chunk_size']
