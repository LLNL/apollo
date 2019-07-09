
#include <mutex>
#include <string>
#include <cstring>

#include "apollo/Apollo.h"
#include "apollo/models/RoundRobin.h"

#include "sos.h"

#define modelName "roundrobin"
#define modelFile __FILE__

int
Apollo::Model::RoundRobin::getIndex(void)
{
    int choice = policy_choice;

    policy_choice++;
    if (policy_choice > policy_count) {
        policy_choice = 1;
    }

    return choice;
}

void
Apollo::Model::RoundRobin::configure(
        Apollo     *apollo_ptr,
        int         num_policies,
        std::string new_model_rule)
{
    apollo        = apollo_ptr;
    policy_count  = num_policies;
    model_def     = new_model_rule;

    int rank = 1;
    char *slurm_procid = getenv("SLURM_PROCID");
    
    if (slurm_procid != NULL) {
       rank = 1 + atoi(slurm_procid);
    } else {
       rank = 1;
    };

    policy_choice = rank % num_policies;

    configured = true;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::RoundRobin::RoundRobin()
{
    name = "RoundRobin";
    training = true;
    iter_count = 0;
}

Apollo::Model::RoundRobin::~RoundRobin()
{
    return;
}

extern "C" Apollo::Model::RoundRobin*
APOLLO_model_create_roundrobin(void)
{
    return new Apollo::Model::RoundRobin;
}


extern "C" void
APOLLO_model_destroy_roundrobin(
        Apollo::Model::RoundRobin *model_ref)
{
    delete model_ref;
    return;
}


