
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "memoryManager.hpp"
#include "RAJA/RAJA.hpp"
//
#include "apollo/Apollo.h"
#include "apollo/PolicyChooser.h"
#include "apollo/Feature.h"
//
#include "addvectLoops.h"

void printProgress(double percentage);

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

    Apollo::Region *experiment  = new Apollo::Region(apollo, "Experiment", 0);
    Apollo::Region *kernel      = new Apollo::Region(apollo, "Kernel", 4);

    //apollo->region("name").begin();

    // 
    // How many times we want to hit these loops
    //
    const int iter_max = 1000;
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
    experiment->caliSetInt("vector_size", N);

    auto vecSize =
        apollo->defineFeature(
            "vector_size",
            Apollo::Goal::OBSERVE,
            Apollo::Unit::INTEGER,
            (void *) &N);

    for (iter_now = 0; iter_now < iter_max; iter_now++) {
        printf("> Iteration %d of %d...\r",
                (iter_now + 1), iter_max);
        fflush(stdout);

        experiment->iterationStart(iter_now);

        // NOTE: This is the behavior we're intending to mimic:
        //
        //RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
        //        c[i] = a[i] + b[i]; 
        //        });
        //
        static int increasing;

        increasing++;

        auto increase = apollo->defineFeature("increasing",
                Apollo::Goal::OBSERVE,
                Apollo::Unit::INTEGER,
                (void *) &increasing);

        kernel->begin();
        addvectPolicySwitcher(
            getApolloPolicyChoice(kernel),
            [=] (auto exec_policy) {
            RAJA::forall(exec_policy, (RAJA::RangeSegment(0, N) ), [=] (int i)
            {
                //Do the work of the kernel here.
                c[i] = a[i] + b[i];
                
            });
        });
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



