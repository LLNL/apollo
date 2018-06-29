
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "memoryManager.hpp"


#include "caliper/cali.h"
#include "RAJA/RAJA.hpp"
//
#include "Apollo.h"

#if defined(RAJA_ENABLE_CUDA)
const int CUDA_BLOCK_SIZE = 256;
#endif

//
// Functions for checking and printing results
//
void checkResult(int* res, int len); 
void printResult(int* res, int len);


int main(int RAJA_UNUSED_ARG(argc), char **RAJA_UNUSED_ARG(argv[]))
{
    CALI_CXX_MARK_FUNCTION;

    std::cout << "\n\nRAJA vector addition example...\n";

    CALI_MARK_BEGIN("initialization");

    //
    // Define vector length
    //
    const int N = 1000000;

    // 
    // How many times we want to hit these loops
    //
    const int iter_max = 10;
    int       iter_now = 0;

    //
    // Allocate and initialize vector data
    //
    int *a = memoryManager::allocate<int>(N);
    int *b = memoryManager::allocate<int>(N);
    int *c = memoryManager::allocate<int>(N);

    for (int i = 0; i < N; ++i) {
        a[i] = -i;
        b[i] = i;
    }

    CALI_MARK_END("initialization");

    printf("\n");

    CALI_CXX_MARK_LOOP_BEGIN(mainloop, "mainloop");
    cali_set_int_byname("vector_size", N);
    cali_set_int_byname("iter_max", iter_max);

    for (iter_now = 0; iter_now < iter_max; iter_now++) {
        printf(">Iteration %d of %d...\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
                (iter_now + 1), iter_max);
        fflush(stdout);

        CALI_CXX_MARK_LOOP_ITERATION(mainloop, iter_now);
        cali_set_int_byname("iter_now", iter_now);


        CALI_CXX_MARK_LOOP_BEGIN(c_forloop, "C:forloop");
        for (int i = 0; i < N; ++i) {
            c[i] = a[i] + b[i];
        }
        CALI_CXX_MARK_LOOP_END(c_forloop);
        //checkResult(c, N);
        //printResult(c, N);

        CALI_CXX_MARK_LOOP_BEGIN(raja_seq, "RAJA:sequential");
        RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        CALI_CXX_MARK_LOOP_END(raja_seq);
        //checkResult(c, N);
        //printResult(c, N);

        CALI_CXX_MARK_LOOP_BEGIN(raja_simd, "RAJA:simd");
        RAJA::forall<RAJA::simd_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        CALI_CXX_MARK_LOOP_END(raja_simd);
        //checkResult(c, N);
        //printResult(c, N);

        CALI_CXX_MARK_LOOP_BEGIN(raja_loopexec, "RAJA:loop_exec");
        RAJA::forall<RAJA::loop_exec>(RAJA::RangeSegment(0, N), [=] (int i) {
                c[i] = a[i] + b[i];
                });
        CALI_CXX_MARK_LOOP_END(raja_loopexec);
        //checkResult(c, N);
        //printResult(c, N);


#if defined(RAJA_ENABLE_OPENMP)
        CALI_CXX_MARK_LOOP_BEGIN(raja_openmp, "RAJA:openmp");
        RAJA::forall<RAJA::omp_parallel_for_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        CALI_CXX_MARK_LOOP_END(raja_openmp);
        //checkResult(c, N);
        //printResult(c, N);
#endif

#if defined(RAJA_ENABLE_CUDA)
        CALI_CXX_MARK_LOOP_BEGIN(raja_cuda, "RAJA:cuda");
        RAJA::forall<RAJA::cuda_exec<CUDA_BLOCK_SIZE>>(RAJA::RangeSegment(0, N), 
                [=] RAJA_DEVICE (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        CALI_CXX_MARK_LOOP_END(raja_cuda);
        //checkResult(c, N);
        //printResult(c, N);
#endif

    }
    CALI_CXX_MARK_LOOP_END(mainloop);
    printf("\n");

    //----------------------------------------------------------------------------//

    //
    // Clean up.
    //
    
    CALI_MARK_BEGIN("deallocation");
    
    memoryManager::deallocate(a);
    memoryManager::deallocate(b);
    memoryManager::deallocate(c);

    CALI_MARK_END("deallocation");

    std::cout << "\n DONE!...\n";

    return 0;
}

//
// Function to check result and report P/F.
//
void checkResult(int* res, int len) 
{
    bool correct = true;
    for (int i = 0; i < len; i++) {
        if ( res[i] != 0 ) { correct = false; }
    }
    if ( correct ) {
        std::cout << "\n\t result -- PASS\n";
    } else {
        std::cout << "\n\t result -- FAIL\n";
    }
}

//
// Function to print result.
//
void printResult(int* res, int len)
{
    std::cout << std::endl;
    for (int i = 0; i < len; i++) {
        std::cout << "result[" << i << "] = " << res[i] << std::endl;
    }
    std::cout << std::endl;
}


