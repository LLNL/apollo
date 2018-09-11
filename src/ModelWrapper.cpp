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
    void *handle = dlopen(path, (RTLD_LAZY | RTLD_GLOBAL));

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
    return true;
}


Apollo::ModelWrapper::ModelWrapper(
        Apollo      *apollo_ptr,
        const char  *path,
        int          numPolicies)
{
    apollo    = apollo_ptr;
    if (loadModel(path) == false) {
        fprintf(stderr, "Unable to load model.\n");
        exit(1);
    }
    model->configure(apollo, numPolicies, "TODO");
    currentPolicyIndex = 0;
    return;
}

int
Apollo::ModelWrapper::requestPolicyIndex(void) {
    currentPolicyIndex = model->getIndex();
    return currentPolicyIndex;
}

Apollo::ModelWrapper::~ModelWrapper() {
    free(modelID);

    destroy( model );
}


