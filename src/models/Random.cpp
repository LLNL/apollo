
#include <random>
#include <mutex>

#include "apollo/Apollo.h"
#include "apollo/models/Random.h"


#define modelName "random"
#define modelFile __FILE__

int
Apollo::Model::Random::getIndex(void)
{
    iterCount++;

    int choice = 0;

    if (policyCount > 1) {
        // Return a random index, 0..N:
        std::random_device rd; 
        std::mt19937 gen(rd()); 
        std::uniform_int_distribution<> dis(0, (policyCount - 1));
        choice = dis(gen);
    } else {
        choice = 0;
    }

    return choice;
}

void
Apollo::Model::Random::configure(
        Apollo *apollo_ptr,
        int numPolicies,
        const char *model_def)
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


Apollo::Model::Random::Random()
{
    iterCount = 0;
}

Apollo::Model::Random::~Random()
{
    return;
}

extern "C" Apollo::Model::Random*
APOLLO_model_create_random(void)
{
    return new Apollo::Model::Random;
}


extern "C" void
APOLLO_model_destroy_random(Apollo::Model::Random *model_ref)
{
    delete model_ref;
    return;
}


