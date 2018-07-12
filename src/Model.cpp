
#include <string.h>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

Apollo::Model::Model(Apollo *apollo_ptr, const char *id_str) {
    apollo = apollo_ptr;
    modelID = strdup(id_str);
    // TODO: Map of regionNames to model_id in the Apollo class.
    //
    modelPattern = strdup("TODO: This is were the learned model will go.");
    currentPolicyIndex = 0;
    return;
}

int
Apollo::Model::requestPolicyIndex(void) {
    // TODO: Interact with SOS to find out what to do.

    int choice = currentPolicyIndex;

    currentPolicyIndex++;
    if (currentPolicyIndex > 5) {
        currentPolicyIndex = 0;
    }

    return choice;
}

Apollo::Model::~Model() {
    free(modelID);
    free(modelPattern);
}


