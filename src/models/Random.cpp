#include <string>
#include <random>
#include <mutex>

#include "apollo/Apollo.h"
#include "apollo/models/Random.h"


#define modelName "random"
#define modelFile __FILE__

int
Apollo::Model::Random::getIndex(void)
{
    iter_count++;

    int choice = 0;

    if (policy_count > 1) {
        std::random_device              random_dev; 
        std::mt19937                    random_gen(random_dev()); 
        std::uniform_int_distribution<> random_dist(0, (policy_count - 1));
        // Return a random index, 0..(policy_count - 1):
        choice = random_dist(random_gen);
    } else {
        choice = 0;
    }

    return choice;
}

void
Apollo::Model::Random::configure(
        Apollo *apollo_ptr,
        int num_policies,
        std::string new_model_def)
{
    apollo = apollo_ptr;
    policy_count = num_policies;
    model_def = new_model_def;
        std::random_device rd; 
        std::mt19937 gen(rd()); 
        std::uniform_int_distribution<> dis(0, (policy_count - 1));
    configured = true;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Random::Random()
{
    name = "Random";
    iter_count = 0;
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


