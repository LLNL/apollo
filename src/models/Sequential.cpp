
#include <mutex>

#include "apollo/Apollo.h"
#include "apollo/models/Sequential.h"

#define modelName "sequential"
#define modelFile __FILE__

int
Apollo::Model::Sequential::getIndex(void)
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
Apollo::Model::Sequential::configure(
        Apollo *apollo_ptr,
        int numPolicies,
        const char *model_def)
{
    apollo = apollo_ptr;
    policyCount = numPolicies;
    configured = true;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Sequential::Sequential()
{

    iterCount = 0;
}

Apollo::Model::Sequential::~Sequential()
{
    return;
}

extern "C" Apollo::Model::Sequential*
APOLLO_model_create_sequential(void)
{
    return new Apollo::Model::Sequential;
}


extern "C" void
APOLLO_model_destroy_sequential(
        Apollo::Model::Sequential *model_ref)
{
    delete model_ref;
    return;
}


