
#include "RAJA/RAJA.hpp"

using rajabenPolicySeq      = RAJA::seq_exec;
using rajabenPolicySIMD     = RAJA::simd_exec;
using rajabenPolicyLoopExec = RAJA::loop_exec;
using rajabenPolicyOpenMP   = RAJA::omp_parallel_for_exec;

#if defined(RAJA_ENABLE_CUDA)
using rajabenPolicyCUDA     = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
const int policy_count = 5;
#else
const int policy_count = 4;
#endif

template <typename BODY>
void rajabenPolicySwitcher(int choice, BODY body) {
    switch (choice) {
    case 1: body(rajabenPolicySeq{});      break;
    case 2: body(rajabenPolicySIMD{});     break;
    case 3: body(rajabenPolicyLoopExec{}); break;
    case 4: body(rajabenPolicyOpenMP{});   break;
#if defined(RAJA_ENABLE_CUDA)
    case 5: body(rajabenPolicyCUDA{});     break;
#endif
    case 0: 
    default: body(rajabenPolicySeq{});     break;
    }
}


