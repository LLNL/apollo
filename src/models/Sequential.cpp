

#include "apollo/Apollo.h"
#include "apollo/Model.h"


// 
// ----------
//
// MODEL: This is where any INDEPENDENT variables get checked
//        and a policy decision is made.
//

#define modelName "sequential"
#define modelFile __FILE__

int
Apollo::Model::getIndex(void)
{
    iterCount++;

    static int choice = -1;

    // Return a sequential index, 0..N:
    choice++;

    if (choice == policyCount) {
        choice = 0;
    }

    return choice;
}

void
Apollo::Model::configure(Apollo *apollo_ptr, int numPolicies, const char *model_def)
{
    apollo = apollo_ptr;
    policyCount = numPolicies;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Model()
{
    iterCount = 0;
}

Apollo::Model::~Model()
{
    return;
}

extern "C" Apollo::Model* create_instance(void)
{
    return new Apollo::Model;
}


extern "C" void destroy_instance(Apollo::Model *model_ref)
{
    delete model_ref;
    return;
}


