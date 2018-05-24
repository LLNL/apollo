
#include <iostream>
#include <random>

#include "stdlib.h"
#include "time.h"

#include "RAJA/RAJA.hpp"

#include "dotprod.hpp"

#define    VERBOSE       1
#define    ARRAY_SIZE    16 

int main() {

    // NOTE: Code use in prior discussion:
    //-----
    //Apollo::Apollo *apollo = new Apollo();
    //Apollo::Region *loop   = new Apollo::Region(apollo);
    //Apollo::Region *loop2  = new Apollo::Region(apollo);
    //Apollo::Region *loop3  = new Apollo::Region(apollo);
    //loop.enter();
    //DotProdPolicySwitcher(
    //        Application::getApolloPolicyChoice(loop), 
    //        [=] (auto exec_policy) {
    //        
    //        // Do work here.
    //        seqdot += a[i] * b[i];
    //
    //        }
    //        );
    //loop.leave();




    // Allocate and initialize our arrays
    int     seed = time(NULL);
    int     N    = ARRAY_SIZE;
    double *a    = (double *) calloc(ARRAY_SIZE, sizeof(double));
    double *b    = (double *) calloc(ARRAY_SIZE, sizeof(double));
    double  dot  = 0.0;

    srand(seed);

    if (VERBOSE) {
        fprintf(stdout, "INPUT:\n\tsrand(%d)\n", seed);
        fprintf(stdout, "\tA[...] ==                 B[...] ==\n");
    }
    for (int i = 0; i < ARRAY_SIZE; i++) {
        a[i] = random_double(0.0, 1.0);
        b[i] = random_double(0.0, 1.0);
        fprintf(stdout, "\t%1.20f    %1.20lf\n",
                a[i], b[i]);
    }
   
    // Set up the Apollo-guided RAJA loop
    RAJA::ReduceSum<RAJA::seq_reduce, double> seqdot(0.0);
    RAJA::forall<RAJA::seq_exec>(RAJA::RangeSegment(0, N), [=] (int i) { 
        seqdot += a[i] * b[i]; 
    });

    dot = seqdot.get();
    if (VERBOSE) fprintf(stdout, "RESULT:\n\tdotprod(a, b) == %1.10lf\n\n", dot);

}

double random_double(double minval, double maxval)
{
    double f = (double)rand() / RAND_MAX;
    return minval + f * (maxval - minval);
}


