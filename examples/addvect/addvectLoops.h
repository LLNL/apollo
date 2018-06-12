
#include "RAJA/RAJA.hpp"

using addvectPolicySeq      = RAJA::seq_exec;
using addvectPolicySIMD     = RAJA::simd_exec;
using addvectPolicyLoopExec = RAJA::loop_exec;
using addvectPolicyOpenMP   = RAJA::omp_parallel_for_exec;

#if defined(RAJA_ENABLE_CUDA)
using addvectPolicyCUDA     = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
#endif

template <typename BODY>
void addvectPolicySwitcher(int choice, BODY body) {
    switch (choice) {
    case 1: body(addvectPolicySeq{}); break;
    case 2: body(addvectPolicySIMD{}); break;
    case 3: body(addvectPolicyLoopExec{}); break;
    case 4: body(addvectPolicyOpenMP{}); break;
#if defined(RAJA_ENABLE_CUDA)
    case 5: body(addvectPolicyCUDA{}); break;
#endif
    case 0: 
    default: body(addvectPolicySeq{}); break;
    }
}

// Beckingsale's recommendation to replace switch statement above:
//
//auto mp = RAJA::make_multi_policy<RAJA::seq_exec, RAJA::omp_parallel_for_exec>(
//    [](const RAJA::RangeSegment &r) {
//        // return 0 for seq, 1 for omp
//        return apollo::getPolicyIndex(); // you implement this
//    });
//
// // pass the multipolicy in to the vector add loop
//forall(mp, ...);

