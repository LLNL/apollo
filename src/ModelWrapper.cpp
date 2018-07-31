#include <dlfcn.h>
#include <string.h>

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"

bool
Apollo::ModelWrapper::loadModel(const char *path) {
    // Clear any prior errors.
    char *error_msg = NULL;
    dlerror();
    
    // Load the shared object:
    void *handle = dlopen(path, RTLD_LAZY);

    if ((error_msg = dlerror()) != NULL) {
        fprintf(stderr, "APOLLO: dlopen(%s, ...): %s\n", path, error_msg);
        return false;
    }

    // Bind to the initialization hooks:
    create = (Apollo::Model* (*)()) dlsym(handle, "create_instance");
    if ((error_msg = dlerror()) != NULL) {
        fprintf(stderr, "APOLLO: dlsym(handle, \"create_instance\") in"
                " shared object %s failed: %s\n", path, error_msg);
        return false;
    }
    
    destroy = (void (*)(Apollo::Model*)) dlsym(handle, "destroy_instance"  );
    if ((error_msg = dlerror()) != NULL) {
        fprintf(stderr, "APOLLO: dlsym(handle, \"destroy_instance\") in"
                " shared object %s failed: %s\n", path, error_msg);
        return false;
    }

    model = NULL;
    model = (Apollo::Model*) create();
    if (model == NULL) {
        fprintf(stderr, "APOLLO: Could not create an instance of the"
                " shared model object %s.\n");
        return false;
    }

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


