// This work was performed under the auspices of the U.S. Department of Energy by
// Lawrence Livermore National Laboratory under Contract DE-AC52-07NA27344.
//

//
//   Tiling modes for different exeuction cases (see luleshPolicy.hxx).
//




enum TilingMode
{
   Canonical,       // canonical element ordering -- single range segment
   Tiled_Index,     // canonical ordering, tiled using unstructured segments
   Tiled_Order,     // elements permuted, tiled using range segments
   Tiled_LockFree,  // tiled ordering, lock-free
   Tiled_LockFreeColor,     // tiled ordering, lock-free, unstructured
   Tiled_LockFreeColorSIMD  // tiled ordering, lock-free, range
};


// Use cases for RAJA execution patterns:

#define LULESH_SEQUENTIAL       1 /* (possible SIMD vectorization applied) */
#define LULESH_CANONICAL        2 /*  OMP forall applied to each for loop */
#define LULESH_CUDA_CANONICAL   9 /*  CUDA launch applied to each loop */
#define LULESH_STREAM_EXPERIMENTAL 11 /* Work in progress... */
#define LULESH_APOLLO           128 /* online ml-driven policy selection */

#ifndef USE_CASE
#define USE_CASE   LULESH_APOLLO
#endif



// ----------------------------------------------------
#if USE_CASE == LULESH_SEQUENTIAL 

TilingMode const lulesh_tiling_mode = Canonical;

typedef RAJA::seq_segit              Segment_Iter;
typedef RAJA::simd_exec              Segment_Exec;

typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> node_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> elem_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> mat_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> symnode_exec_policy;

typedef RAJA::seq_reduce reduce_policy; 

// ----------------------------------------------------
#elif USE_CASE == LULESH_CANONICAL

// Requires OMP_FINE_SYNC when run in parallel
#define OMP_FINE_SYNC 1

// AllocateTouch should definitely be used

TilingMode const lulesh_tiling_mode = Canonical;

typedef RAJA::seq_segit              Segment_Iter;
typedef RAJA::omp_parallel_for_exec  Segment_Exec;

typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> node_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> elem_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> mat_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> symnode_exec_policy;

typedef RAJA::omp_reduce reduce_policy;

// ----------------------------------------------------
#elif USE_CASE == LULESH_CUDA_CANONICAL

// Requires OMP_FINE_SYNC 
#define OMP_FINE_SYNC 1

TilingMode const lulesh_tiling_mode = Canonical;

typedef RAJA::seq_segit         Segment_Iter;

/// Define thread block size for CUDA exec policy
const size_t thread_block_size = 256;
typedef RAJA::cuda_exec<thread_block_size>    Segment_Exec;

typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> node_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> elem_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> mat_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> symnode_exec_policy;

typedef RAJA::cuda_reduce<thread_block_size> reduce_policy;

// ----------------------------------------------------
#elif USE_CASE == LULESH_APOLLO

TilingMode const lulesh_tiling_mode = Canonical;

typedef RAJA::seq_segit         Segment_Iter;
typedef RAJA::apollo_exec       Segment_Exec;

typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> node_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> elem_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> mat_exec_policy;
typedef RAJA::ExecPolicy<Segment_Iter, Segment_Exec> symnode_exec_policy;


//typedef RAJA::ExecPolicy<Segment_Iter, RAJA::apollo_exec> exec_policy;

//typedef exec_policy node_exec_policy;
//typedef exec_policy elem_exec_policy;
//typedef exec_policy mat_exec_policy;
//typedef exec_policy symnode_exec_policy;

typedef RAJA::omp_reduce reduce_policy;

/*
// David P's code:
//
//struct apollo{};
//
//template<typename IndexSet, typename Kernel>
//void apollo_helper(apollo& oracle, IndexSet iset, Kernel kernel){
//    // do Apollo things with oracle
//    // call forall with magically derived policy, passing iset and kernel
//}
//
//#define apollo_forall(...) [&](){\
//  static apollo instance;\
//  apollo_helper(instance,__VA_ARGS__);\
//  }();
//
//struct iset{};
//
//void dbg(){
//    apollo_forall(iset(),[=](){});
//}
//
*/

#else

#error "You must define a use case in luleshPolicy.cxx"

#endif

