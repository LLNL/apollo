#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Region.h"

int getApolloPolicyChoice(Apollo::Region *reg) 
{
    assert (reg != NULL); 
    Apollo::ModelWrapper *model = reg->getModel();
    assert (model != NULL);

    int choice = model->requestPolicyIndex();
    reg->caliSetInt("policyIndex", choice);

    return choice;
}


