
// ##### Application.h

RAJA_INLINE
int getApolloPolicyChoice(Apollo::Region *loop) 
{
    int choice = 0;

    // Here we refer to a stateful element of the Apollo
    // runtime that is doing learning strategies or, having
    // learned the best policy, returning the index.

    if (loop.doneLearning) {
        choice = loop.targetPolicyIndex;
    } else {
        choice = loop.getNextTestPolicy();
    }

    return choice;
}


// In the below code, what gives me concern is passing in
// the "loop" pointer. We don't know what the variable
// name is going to be, yet we need this to be a parameter
// because loops will be in different states or have
// different targetPolicyIndex values.

auto runtime_apollo_policy = 
    RAJA::make_multi_policy<RAJA::seq_exec, 
                            RAJA::loop_exec, 
                            RAJA::simd_exec, 
                            RAJA::omp_parallel_for_exec>
        ([&](const RAJA::TypedRangeSegment<int> &r) {
            (void)(r); // ignore when this parameter is unused
            return getApolloPolicyChoice(loop);
        });





// ##### Application.cpp

#include "Apollo.h"

Apollo *apollo = new Apollo();
Apollo::Region *loop = new Apollo::Region(apollo);

loop.meta("Description", "Some basic description");
loop.meta("Type", "RAJA::nested::forall")

// Idea:
// Express behaviors as lambdas that we want to have inserted into
// loop nesting depths, iterations, or pre/post steering phases.
//
// Everything shown below has obvious and reasonable defaults, and
// in most cases would not need to be set, except for the block
// that is capturing named features.

// Reasoning:
// Some applications are generating output and changing their
// state during the test runs, and we might need to reset things
// once the learning phase is done. A lambda model like this
// gives us the hooks to do so.
//
// It also might be cleaner to say what features we want to capture
// without having to make changes to code inside the RAJA loop,
// supposing C++ scope rules allow the expressions.

// Example lambdas:
loop.on_enter({ loop.getGuidance(); });
loop.on_iteration({});
loop.on_leave({});

// Guidance mode consists of one or more tests.
loop.on_guidance_enter({
    loop.addTarget(apollo.FIND_LOWEST_TIME);
});
loop.on_guidance_test_enter({});
loop.on_guidance_test_iteration({
    loop.note("FeatureX", X);
    loop.note("FeatureY", Y);
    loop.note("FeatureZ", Z);
});
loop.on_guidance_test_leave({
    loop.sendNotes();
});
loop.on_guidance_leave({});


// using JIK_EXECPOL = RAJA::nested::Policy<
//                         RAJA::nested::For<1, RAJA::seq_exec>,
//                         RAJA::nested::For<0, RAJA::seq_exec>,
//                         RAJA::nested::For<2, RAJA::seq_exec> >;
// 
// loop.policy("RAJA::nested::Policy", JIK_EXECPOL);

loop.enter();
RAJA::nested::forall(JIK_EXECPOL{},
        RAJA::make_tuple(IRange, JRange, KRange),
        [=] (IIDX i, JIDX j, KIDX k)
{ 
    // Idea:
    //
    // Eventually, the RAJA multipolicy could be placing the
    // loop.on_interval(...) lambdas in the correct places
    // during code generation. For now we call loop.iterate()
    // and let our code decide when certain things should be done.
    // This may require traversing the RAJA data structures
    // to get the indices and figure out what policy we're using
    // and what the indexes are, to exec the correct lambdas.

    loop.iterate();

    // #####
    // Do the work of the loop...

    printf( " (%d, %d, %d) \n", (int)(*i), (int)(*j), (int)(*k));

    // #####
});
loop.leave();
loop.sendNotes();



