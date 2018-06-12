
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "memoryManager.hpp"
#include "RAJA/RAJA.hpp"
//
#include "Apollo.h"
//
#include "addvectLoops.h"


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

    Apollo::Region *experiment  = new Apollo::Region(apollo, "Experiment");
    Apollo::Region *kernel      = new Apollo::Region(apollo, "Kernel");

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

    experiment->begin();
    experiment->setNamedInt("vector_size", N);

    for (iter_now = 0; iter_now < iter_max; iter_now++) {
        printf("> Iteration %d of %d..."
                "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
                (iter_now + 1), iter_max);
        fflush(stdout);

        experiment->iterationStart(iter_now);
        experiment->setNamedInt("iteration", (iter_now + 1));

        // NOTE: This is the behavior we're intending to mimic:
        //
        //RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
        //        c[i] = a[i] + b[i]; 
        //        });
        //
        kernel->begin();
        addvectPolicySwitcher(
            getApolloPolicyChoice(kernel),
            [=] (auto exec_policy) {
            RAJA::forall<exec_policy>,
                (RAJA::RangeSegment(0, N) ),
                [=] (int i) {
                    //Do the work of the kernel here.
                    c[i] = a[i] + b[i];                        
                
                };
            }
        );
        kernel->end();
        //
        //

        experiment->iterationStop();
    }

    experiment->end();
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


