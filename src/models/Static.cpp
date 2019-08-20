
#include <mutex>
#include <string>
#include <cstring>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/models/Static.h"

#define modelName "static"
#define modelFile __FILE__

int
Apollo::Model::Static::getIndex(void)
{
    iter_count++;

    return policy_choice;
}

void
Apollo::Model::Static::configure(
        int  num_policies,
        json new_model_rule)
{
    apollo        = Apollo::instance();
    policy_count  = num_policies;
    model_def     = new_model_rule;

    policy_choice = std::stoi(model_def.get<std::string>());

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
    name = "Static";
    training = false;
    iter_count = 0;
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


