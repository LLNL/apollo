#include <string>
#include <mutex>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/models/Sequential.h"

#define modelName "sequential"
#define modelFile __FILE__

int
Apollo::Model::Sequential::getIndex(void)
{
    iter_count++;

    static int choice = -1;

    // Return a sequential index, 0..N:
    choice++;

    if (choice == policy_count) {
        choice = 0;
    }

    return choice;
}

void
Apollo::Model::Sequential::configure(
        int  num_policies,
        json new_model_def)
{
    apollo        = Apollo::instance();
    policy_count  = num_policies;
    model_def     = new_model_def;
    configured    = true;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Sequential::Sequential()
{
    name = "Sequential";
    iter_count = 0;
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


