#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Region.h"
 
RAJA_INLINE
int getApolloPolicyChoice(Apollo::Region *loop) 
{
    assert (loop != NULL); 
    Apollo::ModelWrapper *model = loop->getModel();
    assert (model != NULL);

    int choice = model->requestPolicyIndex();
    loop->caliSetInt("policyIndex", choice);

    return choice;
}


