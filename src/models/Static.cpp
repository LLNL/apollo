
#include <mutex>
#include <cstring>

#include "apollo/Apollo.h"
#include "apollo/models/Static.h"

#define modelName "static"
#define modelFile __FILE__

int
Apollo::Model::Static::getIndex(void)
{
    iterCount++;

    return policyChoice;
}

void
Apollo::Model::Static::configure(
        Apollo *apollo_ptr,
        int numPolicies,
        const char *model_def)
{
    apollo = apollo_ptr;
    policyCount = numPolicies;
    policyChoice = atoi(model_def);

    configured = true;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Static::Static()
{

    iterCount = 0;
}

Apollo::Model::Static::~Static()
{
    return;
}

extern "C" Apollo::Model::Static*
APOLLO_model_create_static(void)
{
    return new Apollo::Model::Static;
}


extern "C" void
APOLLO_model_destroy_static(
        Apollo::Model::Static *model_ref)
{
    delete model_ref;
    return;
}


