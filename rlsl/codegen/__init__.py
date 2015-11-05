import numpy as np

POLICY_LINE = {
    'CILK_FOR CILK_FOR' : 'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::cilk_for_exec> >(iset, loop_body)',
    'OMP_SEGIT SEG_CILK' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_for_segit, RAJA::cilk_for_exec> >(iset, loop_body)',
    'SEQ_SEGIT CILK_FOR' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::cilk_for_exec> >(iset, loop_body)',
    'CILK_SEGIT SEG_OMP' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::omp_parallel_for_exec> >(iset, loop_body)',
    'SEQ_SEGIT OMP_PARALLEL' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::omp_parallel_for_exec> >(iset, loop_body)',
    'OMP_SEGIT OMP_PARALLEL' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_for_segit, RAJA::omp_parallel_for_exec> >(iset, loop_body)',
    'CILK_SEGIT SEG_SEQ' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::seq_exec> >(iset, loop_body)',
    'OMP_SEGIT SEG_SEQ' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_for_segit, RAJA::seq_exec> >(iset, loop_body)',
    'SEQ_SEGIT SEQ_EXEC' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::seq_exec> >(iset, loop_body)',
    'CILK_FOR SEG_SIMD' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::cilk_for_segit, RAJA::simd_exec> >(iset, loop_body)',
    'OMP_SEGIT SEG_SIMD' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::omp_parallel_fon_segit, RAJA::simd_exec> >(iset, loop_body)',
    'SEQ_SEGIT SEG_SIMD' :  'RAJA::forall<RAJA::IndexSet::ExecPolicy<RAJA::seq_segit, RAJA::simd_exec> >(iset, loop_body)'
}

class CodeGenerator(object):
    def get_code(self, tree, feature_names, target_names, spacer_base="  ", loop_id_labels=None):
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

        print tree.tree_.feature
        print features
        print target_names

        def recurse(left, right, threshold, features, node, depth):
            spacer = spacer_base * depth
            if (threshold[node] != -2):
                if 'loop_id' in features[node]:
                    print features[node], threshold[node]
                    #print spacer + "if ( " + features[node].replace(' ', '_') + " == " + loop_id_labels.classes_[str(threshold[node])] + " ) {"
                else:
                    print spacer + "if ( " + features[node].replace(' ', '_') + " <= " + str(threshold[node]) + " ) {"

                if left[node] != -1:
                        recurse(left, right, threshold, features,
                                left[node], depth+1)
                print spacer + "}\n" + spacer +"else {"
                if right[node] != -1:
                        recurse(left, right, threshold, features,
                                right[node], depth+1)
                print spacer + "}"
            else:
                target = value[node]
                max_target = np.argmax(target)
                #print target

                #for i, v in zip(np.nonzero(target)[1],
                #                target[np.nonzero(target)]):
                target_name = target_names[max_target]
                #print target_names[max_target]
                #policy = target_name['exec_it_best']
                print spacer + str(POLICY_LINE[target_name]) + ';'

        recurse(left, right, threshold, features, 0, 0)
