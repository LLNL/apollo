
#include <string>

#include "Python.h"

#include "apollo/Apollo.h"
#include "apollo/models/Python.h"

#define modelName "python"
#define modelFile __FILE__

// PURPOSE:
//     This is a stub at the moment, but eventually this is
//     is intended to be a "general utility" model that executers
//     arbitrary Python code supplied by the controller.


int
Apollo::Model::Python::getIndex(void)
{
    iter_count++;
    static int choice = -1;

    // TODO: Grab the python string.
    //       Grab the independent variables and stringify them.
    //       Concatenate the strings.
    //       Execute the python.
    //       Capture the return value into (int) choice.

    return choice;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//

void
Apollo::Model::Python::configure(
        Apollo       *apollo_ptr,
        int           num_policies,
        std::string   new_model_def)
{
    apollo       = apollo_ptr;
    policy_count = num_policies;
    model_def    = new_model_def;
    configured   = true;
    return;
}

Apollo::Model::Python::Python()
{
    iter_count = 0;
}

Apollo::Model::Python::~Python()
{
    return;
}

extern "C" Apollo::Model::Python*
APOLLO_model_create_python(void)
{
    return new Apollo::Model::Python;
}


extern "C" void
APOLLO_model_destroy_python(Apollo::Model *model_ref)
{
    delete model_ref;
    return;
}


