
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "memoryManager.hpp"
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

    std::cout << "\n\nRAJA vector addition example.\n";

    Apollo *apollo = new Apollo();

    Apollo::Region *loopAllTests    = new Apollo::Region(apollo, "alltests");

    Apollo::Region *loopCStyle      = new Apollo::Region(apollo, "C:forloop");
    Apollo::Region *loopSequential  = new Apollo::Region(apollo, "RAJA:sequential");
    Apollo::Region *loopSIMD        = new Apollo::Region(apollo, "RAJA:simd");
    Apollo::Region *loopExec        = new Apollo::Region(apollo, "RAJA:loop_exec");
    Apollo::Region *loopOpenMP      = new Apollo::Region(apollo, "RAJA:openmp");
    Apollo::Region *loopCUDA        = new Apollo::Region(apollo, "RAJA:cuda");

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

    printf("\n");

    loopAllTests->begin();
    loopAllTests->setNamedInt("vector_size", N);

    for (iter_now = 0; iter_now < iter_max; iter_now++) {
        printf("> Iteration %d of %d..."
                "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
                (iter_now + 1), iter_max);
        fflush(stdout);

        loopAllTests->iterationStart(iter_now);
        loopAllTests->setNamedInt("iteration", (iter_now + 1));

        loopCStyle->begin();
        for (int i = 0; i < N; ++i) {
            c[i] = a[i] + b[i];
        }
        loopCStyle->end();
        

        loopSequential->begin();
        RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        loopSequential->end();


        loopSIMD->begin();
        RAJA::forall<RAJA::simd_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        loopSIMD->end();


        loopExec->begin();
        RAJA::forall<RAJA::loop_exec>(RAJA::RangeSegment(0, N), [=] (int i) {
                c[i] = a[i] + b[i];
                });
        loopExec->end();


#if defined(RAJA_ENABLE_OPENMP)
        loopOpenMP->begin();
        RAJA::forall<RAJA::omp_parallel_for_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        loopOpenMP->end();
#endif

#if defined(RAJA_ENABLE_CUDA)
        loopCUDA->begin();
        RAJA::forall<RAJA::cuda_exec<CUDA_BLOCK_SIZE>>(RAJA::RangeSegment(0, N), 
                [=] RAJA_DEVICE (int i) { 
                c[i] = a[i] + b[i]; 
                });    
        loopCUDA->end();
#endif

        loopAllTests->iterationStop();
    }

    loopAllTests->end();
    printf("\n");

    //----------------------------------------------------------------------------//

    //
    // Clean up.
    //
    
    
    memoryManager::deallocate(a);
    memoryManager::deallocate(b);
    memoryManager::deallocate(c);


    std::cout << "\n DONE.\n";

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


