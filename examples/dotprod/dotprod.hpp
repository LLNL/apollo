#ifndef APOLLO_EXAMPLES_DOTPROD_HPP
#define APOLLO_EXAMPLES_DOTPROD_HPP

#include "Apollo.h"
#include "RAJA/RAJA.hpp"

double random_double(double minval, double maxval);

RAJA_INLINE
int getApolloPolicyChoice() 
{
    int choice = 0;

    // Here we refer to a stateful element of the Apollo
    // runtime that is doing learning strategies or, having
    // learned the best policy, returning the index.

    // TODO: Perform cursory analysis here:
    // 
    // -----

    if (true) {
        choice = 0;
    } else {
        choice = 1;
    }

    return choice;
}

//  Set up a list of concrete policies
using Policy_DotProd_Seq = RAJA::nested::Policy<
	RAJA::nested::TypedFor<0, RAJA::loop_exec, Moment>,
	RAJA::nested::TypedFor<1, RAJA::loop_exec, Direction>,
	RAJA::nested::TypedFor<2, RAJA::loop_exec, Group>,
	RAJA::nested::TypedFor<3, RAJA::loop_exec, Zone>
	>;
using Policy_DotProd_Omp = RAJA::nested::Policy<
	RAJA::nested::TypedFor<2, RAJA::omp_parallel_for_exec, Group>,
	RAJA::nested::TypedFor<0, RAJA::loop_exec, Moment>,
	RAJA::nested::TypedFor<1, RAJA::loop_exec, Direction>,
	RAJA::nested::TypedFor<3, RAJA::loop_exec, Zone>
	>;

// STEP 2 of 3: Make a template that uses some choice
//              to switch amongst the concrete policies:
template <typename BODY>
void DotProdPolicySwitcher(int choice, BODY body) {
    switch (choice) {
    case 1: body(Policy_DotProd_Omp{}); break;
    case 0: 
    default: body(Policy_DotProd_Seq{}); break;
    }
}









#endif



