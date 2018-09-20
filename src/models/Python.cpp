
#include "Python.h"

#include "apollo/Apollo.h"
#include "apollo/models/Python.h"

#define modelName "python"
#define modelFile __FILE__

int
Apollo::Model::Python::getIndex(void)
{
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
Apollo::Model::Python::configure(Apollo *apollo_ptr, int numPolicies, const char *model_conf_def)
{
    apollo = apollo_ptr;
    policyCount = numPolicies;
    if (model_conf_def != NULL) {
        model_def = model_conf_def;
    } else {
        model_def = "";
    }
    return;
}

Apollo::Model::Python::Python()
{
    iterCount = 0;
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


