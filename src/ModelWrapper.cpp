#include <dlfcn.h>
#include <string.h>

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"

bool
Apollo::ModelWrapper::loadModel(const char *path, const char *definition) {
    // Grab the object lock so if we're changing models
    // in the middle of a run, the client wont segfault
    // attempting to region->requestPolicyIndex() at the
    // head of a loop.
    std::lock_guard<std::mutex> lock(object_lock);

    if (object_loaded) {
        // TODO: Clean up after prior model.
    }
   
    object_loaded = false;

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

    model->configure(apollo, num_policies, definition);

    object_loaded = true;
    return object_loaded;
}


Apollo::ModelWrapper::ModelWrapper(
        Apollo      *apollo_ptr,
        int          numPolicies)
{
    apollo = apollo_ptr;
    num_policies = numPolicies;

    const char  *path       = APOLLO_DEFAULT_MODEL_OBJECT_PATH;
    const char  *definition = APOLLO_DEFAULT_MODEL_DEFINITION;

    loadModel(path, definition);

    if (object_loaded == false) {
        fprintf(stderr, "ERROR: Unable to load model.  (%s)\n", path);
    }

    return;
}

int
Apollo::ModelWrapper::requestPolicyIndex(void) {
    // Grab the object lock to prevent segfaults if Apollo
    // is changing the model behind the scenes as we're coming
    // into a new RAJA loop and requesting a policy index.
    std::lock_guard<std::mutex> lock(object_lock);

    static int err_count = 0;
    if (object_loaded == false) {
        err_count++;
        if (err_count < 10) {
            fprintf(stderr, "WARNING: requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default).\n");
            return 0;
        } else if (err_count == 10) {
            fprintf(stderr, "WARNING: requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default) and suppressing"
                    " additional identical error messages.\n");
            return 0;
        }
        return 0;
    }
    // Actually call the model now:
    return model->getIndex();
}

Apollo::ModelWrapper::~ModelWrapper() {
    id = "";
    destroy(model);
}


