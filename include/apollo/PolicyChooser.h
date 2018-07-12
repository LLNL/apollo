
#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"
 
RAJA_INLINE
int getApolloPolicyChoice(Apollo::Region *loop) 
{
    int choice = 0;

    if (loop != NULL) {
        Apollo::Model *model = loop->getModel();
        if (model != NULL) {
            choice = model->requestPolicyIndex();
        }
    }

    loop->caliSetInt("policyIndex", choice);

    return choice;
}


