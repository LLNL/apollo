
#include "RAJA/RAJA.hpp"

using apollo2DwSeqPolicySeq      = RAJA::seq_exec;
using apollo2DwSeqPolicySIMD     = RAJA::simd_exec;
using apollo2DwSeqPolicyLoopExec = RAJA::loop_exec;
#if defined(RAJA_ENABLE_OPENMP)
  using apollo2DwSeqPolicyOpenMP   = RAJA::omp_parallel_for_exec;
  #if defined(RAJA_ENABLE_CUDA)
    using apollo2DwSeqPolicyCUDA     = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
    const int POLICY_COUNT = 5;
  #else
    const int POLICY_COUNT = 4;
  #endif
#else
  #if defined(RAJA_ENABLE_CUDA)
    using apollo2DwSeqPolicyCUDA     = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
    const int POLICY_COUNT = 4;
  #else
    const int POLICY_COUNT = 3;
  #endif
#endif

template <typename BODY>
RAJA_INLINE void apollo2DwSeqPolicySwitcher(int choice, BODY body) {
    switch (choice) {
    case 1: body(apollo2DwSeqPolicySeq{});      break;
    case 2: body(apollo2DwSeqPolicySIMD{});     break;
    case 3: body(apollo2DwSeqPolicyLoopExec{}); break;
#if defined(RAJA_ENABLE_OPENMP)
    case 4: body(apollo2DwSeqPolicyOpenMP{});   break;
    #if defined(RAJA_ENABLE_CUDA)
    case 5: body(apollo2DwSeqPolicyCUDA{});     break;
    #endif
#else
    #if defined(RAJA_ENABLE_CUDA)
    case 4: body(apollo2DwSeqPolicyCUDA{});     break;
    #endif
#endif
    case 0: 
    default: body(apollo2DwSeqPolicySeq{});     break;
    }
}

template <typename Iterable, typename Func>
RAJA_INLINE void apollo_exec_2DwSeq(const apollo_exec &, Iterable &&iter, Func &&body)
{
    static Apollo::Region *apolloRegion      = nullptr;
    static int             apolloExecCount   = 0;
    static int             policyIndex       = 0;
    if (apolloRegion == nullptr) {
        // ----------
        // NOTE: This section runs *once* the first time the
        //       region is encountered
        std::stringstream ss_location;
        ss_location << (const void *) &body;
        apolloRegion = new Apollo::Region(
            Apollo::instance(),
            ss_location.str().c_str(),
            RAJA::policy::apollo::POLICY_COUNT);
        // ----------
    }


    apolloRegion->begin(apolloExecCount++);
    
    policyIndex = apolloRegion->getPolicyIndex();
    
    apolloPolicySwitcher(policyIndex , [=] (auto pol) {
        forall_impl(pol, iter, body); });
    
    apolloRegion->end();
    
}

