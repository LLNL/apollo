
#include <memory>
#include <mutex>

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
//
#include "apollo/models/Random.h"
#include "apollo/models/Sequential.h"
#include "apollo/models/DecisionTree.h"
#include "apollo/models/Python.h"

bool
Apollo::ModelWrapper::configure(
        int                   model_type,
        const char           *model_def)
{
    Apollo::Model::Type MT;

    if (model_type == MT.Default) { model_type = APOLLO_DEFAULT_MODEL_TYPE; }
    std::shared_ptr<Apollo::ModelObject> nm = nullptr;

    switch (model_type) {
        //
        case MT.Random:       nm = std::make_shared<Apollo::Model::Random>();       break;
        case MT.Sequential:   nm = std::make_shared<Apollo::Model::Sequential>();   break;
        case MT.DecisionTree: nm = std::make_shared<Apollo::Model::DecisionTree>(); break;
        case MT.Python:       nm = std::make_shared<Apollo::Model::Python>();       break;
        //
        default:
             fprintf(stderr, "WARNING: Unsupported Apollo::Model::Type"
                     " specified to Apollo::ModelWrapper::configure."
                     " Doing nothing.\n");
             return false;
    }

    Apollo::ModelObject *lnm = nm.get();
    lnm->configure(apollo, num_policies, model_def);

    model_sptr.reset(); // Release ownership of the prior model's shared ptr
    model_sptr = nm;    // Make this new model available for use.

    return true;
}


Apollo::ModelWrapper::ModelWrapper(
        Apollo      *apollo_ptr,
        int          numPolicies)
{
    apollo = apollo_ptr;
    num_policies = numPolicies;

    model_sptr = nullptr;

    return;
}

// NOTE: This is the method that RAJA loops call, they don't
//       directly call the model's getIndex() method.
int
Apollo::ModelWrapper::requestPolicyIndex(void) {
    // Claim shared ownership of the current model object.
    // NOTE: This is useful in case Apollo replaces the model with
    //       something else while we're in this [model's] method. The model
    //       we're picking up here will not be destroyed until
    //       this (and all other co-owners) are done with it,
    //       though Apollo is not prevented from setting up
    //       a new model that other threads will be getting, all
    //       without global mutex synchronization.
    std::shared_ptr<Apollo::ModelObject> lm_sptr = model_sptr;
    Apollo::ModelObject *model = lm_sptr.get();

    static int err_count = 0;
    if (model == nullptr) {
        err_count++;
        lm_sptr.reset();
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
    int choice = model->getIndex();

    return choice;
}

Apollo::ModelWrapper::~ModelWrapper() {
    id = "";
    model_sptr.reset(); // Release access to the shared object.
}


// // NOTE: This is deprecated in favor of "model processing engines"
// //       that are built into the libapollo.so
//
// #include <dlfcn.h>
// #include <string.h>
//
// bool
// Apollo::ModelWrapper::loadModel(const char *path, const char *definition) {
//     // Grab the object lock so if we're changing models
//     // in the middle of a run, the client wont segfault
//     // attempting to region->requestPolicyIndex() at the
//     // head of a loop.
//     std::lock_guard<std::mutex> lock(object_lock);
// 
//     if (object_loaded) {
//         // TODO: Clean up after prior model.
//     }
//    
//     object_loaded = false;
// 
//     // Clear any prior errors.
//     char *error_msg = NULL;
//     dlerror();
//     
//     // Load the shared object:
//     void *handle = dlopen(path, (RTLD_LAZY | RTLD_GLOBAL));
// 
//     if ((error_msg = dlerror()) != NULL) {
//         fprintf(stderr, "APOLLO: dlopen(%s, ...): %s\n", path, error_msg);
//         return false;
//     }
// 
//     // Bind to the initialization hooks:
//     create = (Apollo::Model* (*)()) dlsym(handle, "create_instance");
//     if ((error_msg = dlerror()) != NULL) {
//         fprintf(stderr, "APOLLO: dlsym(handle, \"create_instance\") in"
//                 " shared object %s failed: %s\n", path, error_msg);
//         return false;
//     }
//     
//     destroy = (void (*)(Apollo::Model*)) dlsym(handle, "destroy_instance"  );
//     if ((error_msg = dlerror()) != NULL) {
//         fprintf(stderr, "APOLLO: dlsym(handle, \"destroy_instance\") in"
//                 " shared object %s failed: %s\n", path, error_msg);
//         return false;
//     }
// 
//     model = NULL;
//     model = (Apollo::Model*) create();
//     if (model == NULL) {
//         fprintf(stderr, "APOLLO: Could not create an instance of the"
//                 " shared model object %s.\n");
//         return false;
//     }
// 
//     model->configure(apollo, num_policies, definition);
// 
//     object_loaded = true;
//     return object_loaded;
// }
// 


