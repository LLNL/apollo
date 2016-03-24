import numpy as np

POLICY_LINE = {
    'cilk_for_cilk_for' : 'body(RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::cilk_for_exec>())',
    'omp_segit_seg_cilk' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_for_segit, RAJA::cilk_for_exec>())',
    'seq_segit_cilk_for' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::cilk_for_exec>())',
    'cilk_segit_seg_omp' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::omp_parallel_for_exec>())',
    'seq_segit_omp_parallel_for_exec' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::omp_parallel_for_exec>())',
    'omp_parallel_for_segit_omp_parallel_for_exec' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_for_segit, RAJA::omp_parallel_for_exec>())',
    'cilk_segit_seg_seq' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::seq_exec>())',
    'omp_parallel_for_segit_seq_exec' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_for_segit, RAJA::seq_exec>())',
    'seq_segit_seq_exec' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::seq_exec>())',
    'cilk_for_seg_simd' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::simd_exec>())',
    'omp_segit_seg_simd' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_fon_segit, RAJA::simd_exec>())',
    'seq_segit_seg_simd' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::simd_exec>())'
}

RANGE_LINE = {
    'seq_segit_omp_parallel_for_exec' :  'body(RAJA::omp_parallel_for_exec())',
    'omp_parallel_for_segit_omp_parallel_for_exec' :  'body(RAJA::omp_parallel_for_exec())',
    'omp_parallel_for_segit_seq_exec' :  'body(RAJA::seq_exec())',
    'seq_segit_seq_exec' :  'body(RAJA::IndexSet::ExecPolicy<RAJA::seq_exec())',
}

class CodeGenerator(object):
    def __init__(self, kind):
        self.kind = kind
        self.model = ""

    def get_code(self, tree, feature_names, target_names, instruction_names, spacer_base="  ", loop_id_labels=None):
        """Produce psuedo-code for decision tree.

        Args
        ----
        tree -- scikit-learnt DescisionTree.
        feature_names -- list of feature names.
        target_names -- list of target (class) names.
        spacer_base -- used for spacing code (default: "    ").

        Notes
        -----
        based on http://stackoverflow.com/a/30104792.
        """
        left      = tree.tree_.children_left
        right     = tree.tree_.children_right
        threshold = tree.tree_.threshold
        features  = [feature_names[i] if (i >= 0) else 'true' for i in tree.tree_.feature]
        value = tree.tree_.value

        #print tree.tree_.feature
        #print features
        #print target_names

        self.instruction_map_line = False

        for f in np.unique(features):
            if f in instruction_names:
                if not self.instruction_map_line:
                    self.model += "auto instruction_map = kavalier::KavalierStatistician::getManager().getInstructionCounts(Dyninst::Address(func), *static_cast<uint64_t *>(func));"
                else:
                    pass
            elif f == 'true':
                pass
            else:
                self.model += 'int %s = s.get(c.get_attribute("%s")).value().to_int();\n' % (f,f)

        def recurse(left, right, threshold, features, node, depth):
            spacer = spacer_base * depth
            if (threshold[node] != -2):
                if 'loop_id' in features[node]:
                     self.model += features[node] + ' ' + threshold[node] + '\n'
                    #print spacer + "if ( " + features[node].replace(' ', '_') + " == " + loop_id_labels.classes_[str(threshold[node])] + " ) {"
                else:
                    if features[node] in instruction_names:
                        self.model += spacer + 'if ( instruction_map["' +features[node].replace(' ', '_') + '"] <= ' + str(threshold[node]) + " ) {\n"
                    else:
                        self.model += spacer + "if ( " + features[node].replace(' ', '_') + " <= " + str(threshold[node]) + " ) {\n"

                if left[node] != -1:
                        recurse(left, right, threshold, features,
                                left[node], depth+1)
                self.model += spacer + "}\n" + spacer +"else {\n"
                if right[node] != -1:
                        recurse(left, right, threshold, features,
                                right[node], depth+1)
                self.model += spacer + "}\n"
            else:
                target = value[node]
                max_target = np.argmax(target)
                target_name = target_names[max_target]

                if 'policy' in self.kind:
                    self.model += spacer + "p.policy = RAJA::apollo::" + target_name + ';\n'
                elif 'chunk' in self.kind:
                    self.model += spacer + "p.chunk_size = " + str(target_name) + ';\n'
                elif 'fraction' in self.kind:
                    self.model += spacer + "p.dynamic_fraction = " + str(target_name) + ';\n'

        recurse(left, right, threshold, features, 0, 0)

        return self.model
