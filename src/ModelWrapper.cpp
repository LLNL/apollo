#include <dlfcn.h>
#include <string.h>

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"

bool
Apollo::ModelWrapper::loadModel(const char *path) {
    void *handle = dlopen(path, RTLD_LAZY);

    // Bind to the initialization hooks:
    create  = (Apollo::Model* (*)()) dlsym(handle,      "create_instance"   );
    destroy = (void (*)(Apollo::Model*)) dlsym(handle, "destroy_instance"  );

    model = (Apollo::Model*) create();

    model->configure(apollo); 
    
    return true;
}



Apollo::ModelWrapper::ModelWrapper(Apollo *apollo_ptr, const char *path) {
    apollo    = apollo_ptr;
    modelPath = strdup("TODO: This is were the learned model will go.");
    currentPolicyIndex = 0;
    return;
}

int
Apollo::ModelWrapper::requestPolicyIndex(void) {
    // TODO: Interact with SOS to find out what to do.

    int choice = currentPolicyIndex;

    currentPolicyIndex++;
    if (currentPolicyIndex > 5) {
        currentPolicyIndex = 0;
    }

    return choice;
}

Apollo::ModelWrapper::~ModelWrapper() {
    free(modelID);

    destroy( model );
}


