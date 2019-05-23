
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
//
#include "apollo/models/Random.h"
#include "apollo/models/Sequential.h"
#include "apollo/models/Static.h"
#include "apollo/models/DecisionTree.h"


using namespace std;

std::string
Apollo::ModelWrapper::getName(void)
{
    Apollo::ModelObject *lnm = model_sptr.get();
    return lnm->name;
}

uint64_t
Apollo::ModelWrapper::getGuid(void)
{
    Apollo::ModelObject *lnm = model_sptr.get();
    return lnm->getGuid();
}

int
Apollo::ModelWrapper::getPolicyCount(void)
{
    return num_policies;
}

bool
Apollo::ModelWrapper::isTraining(void)
{
    Apollo::ModelObject *lnm = model_sptr.get();
    return lnm->training;
}

bool
Apollo::ModelWrapper::configure(
        const char           *model_def)
{
    Apollo::Model::Type MT;
    int model_type = -1;

    if (model_def == NULL) {
        model_type = MT.Default;
    } else if (strlen(model_def) < 1) {
        model_type = MT.Default;
    }

    if (model_type == MT.Default) {
        model_def  = MT.DefaultConfigJSON;
        apollo_log(2, "Using the default model for initialization.\n");
    }

    if (getenv("APOLLO_INIT_MODEL") != NULL) {
        model_def = getenv("APOLLO_INIT_MODEL");
        apollo_log(2, "Using ${APOLLO_INIT_MODEL} for region initialization.\n");
    }

    apollo_log(9, "Model definition:\n%s\n", model_def);

    // Extract the various common elements from the model definition
    // and provide them to the configure method, independent of whatever
    // further definitions are unique to that model.
    json j = json::parse(model_def);

    uint64_t       m_type_guid;
    string         m_type_name;
    vector<string> m_region_names;
    int            m_feat_count;
    vector<string> m_feat_names;
    string         m_drv_format;
    string         m_drv_rules;

    apollo_log(3, "Attempting to parse model definition...\n");
    
    // Validate and extract model components:
    int model_errors = 0;
    apollo_log(3, "\t[type]\n");
    if (j.find("type") == j.end()) {
        apollo_log(1, "Invalid model_def: missing [type]\n");
        model_errors++;
    } else {
        apollo_log(3, "\t[type][guid]\n");
        if (j["type"].find("guid") == j["type"].end()) {
            apollo_log(1, "Invalid model_def: missing [type][guid]\n");
            model_errors++;
        } else {
            m_type_guid = j["type"]["guid"].get<uint64_t>();
        }
        apollo_log(3, "\t[type][name]\n");
        if (j["type"].find("name") == j["type"].end()) {
            apollo_log(1, "Invalid model_def: missing [type][name]\n");
            model_errors++;
        } else {
            m_type_name = j["type"]["name"].get<string>();
        }
    }
    apollo_log(3, "\t[region_names]\n");
    if (j.find("region_names") == j.end()) {
        apollo_log(1, "Invalid model_def: missing [region_names]\n");
        model_errors++;
    } else {
        m_region_names = j["region_names"].get<vector<string>>();
    }
    
   
    apollo_log(3, "\t[features]\n");
    if (j.find("features") == j.end()) {
        apollo_log(1, "Invalid model_def: missing [features]\n");
        model_errors++;
    } else {
        apollo_log(3, "[features][count]\n");
        if (j["features"].find("count") == j["features"].end()) {
            apollo_log(1, "Invalid model_def: missing [features][count]\n");
            model_errors++;
        } else {
            m_feat_count = j["features"]["count"].get<int>();
        }
        apollo_log(3, "\t[features][names]\n");
        if (j["features"].find("names") == j["features"].end()) {
            apollo_log(1, "Invalid model_def: missing [features][names]\n");
            model_errors++;
        } else {
            m_feat_names = j["features"]["names"].get<vector<string>>();
        }
    }
    apollo_log(3, "\t[driver]\n");
    if (j.find("driver") == j.end()) {
        apollo_log(1, "Invalid model_def: missing [driver]\n");
        model_errors++;
    } else {
        apollo_log(3, "\t[driver][format]\n");
        if (j["driver"].find("format") == j["driver"].end()) {
            apollo_log(1, "Invalid model_def: missing [driver][format]\n");
            model_errors++;
        } else {
            m_drv_format = j["driver"]["format"].get<string>();
        }
        apollo_log(3, "\t[driver][rules]\n");
        if (j["driver"].find("rules") == j["driver"].end()) {
            apollo_log(1, "Invalid model_def: missing [driver][rules]\n");
            model_errors++;
        } else {
            m_drv_rules = j["driver"]["rules"].get<string>();
        }
    }

    if (model_errors > 0) {
        fprintf(stderr, "== APOLLO: [ERROR] There were %d errors parsing"
                " the supplied model definition.\n", model_errors);
        exit(1);
    }

    if      (m_type_name == "Random")       { model_type = MT.Random; }
    else if (m_type_name == "Sequential")   { model_type = MT.Sequential; }
    else if (m_type_name == "Static")       { model_type = MT.Static; }
    else if (m_type_name == "DecisionTree") { model_type = MT.DecisionTree; }
    else                                    { model_type = MT.Default; }

    if (model_type == MT.Default) { model_type = APOLLO_DEFAULT_MODEL_TYPE; }
    shared_ptr<Apollo::ModelObject> nm = nullptr;

    switch (model_type) {
        //
        case MT.Random:
            apollo_log(2, "Applying new Apollo::Model::Random()\n");
            nm = make_shared<Apollo::Model::Random>();
            break;
        case MT.Sequential:
            apollo_log(2, "Applying new Apollo::Model::Sequential()\n");
            nm = make_shared<Apollo::Model::Sequential>();
            break;
        case MT.Static:
            apollo_log(2, "Applying new Apollo::Model::Static()\n");
            nm = make_shared<Apollo::Model::Static>();
            break;
        case MT.DecisionTree:
            apollo_log(2, "Applying new Apollo::Model::DecisionTree()\n");
            nm = make_shared<Apollo::Model::DecisionTree>();
            break;
        //
        default:
             fprintf(stderr, "== APOLLO: [WARNING] Unsupported Apollo::Model::Type"
                     " specified to Apollo::ModelWrapper::configure."
                     " Doing nothing.\n");
             return false;
    }

    Apollo::ModelObject *lnm = nm.get();

    switch (model_type) {
        case MT.Static:
        case MT.DecisionTree:
            lnm->training = false;
            break;
            //
        case MT.Random:
        case MT.Sequential:
        default:
            lnm->training = true;
            break;
    }

    lnm->configure(apollo, num_policies, m_drv_rules);
    lnm->setGuid(m_type_guid);

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
    shared_ptr<Apollo::ModelObject> lm_sptr = model_sptr;
    Apollo::ModelObject *model = lm_sptr.get();

    static int err_count = 0;
    if (model == nullptr) {
        err_count++;
        lm_sptr.reset();
        if (err_count < 10) {
            apollo_log(0, "[WARNING] requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default).\n");
            return 0;
        } else if (err_count == 10) {
            apollo_log(0, "[WARNING] requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default) and suppressing"
                    " additional identical error messages.\n");
            return 0;
        }
        return 0;
    } else {
        err_count = 0;
    }

    // Actually call the model now:
    int choice = model->getIndex();

    return choice;
}

Apollo::ModelWrapper::~ModelWrapper() {
    id = "";
    model_sptr.reset(); // Release access to the shared object.
}


// // NOTE: This is deprecated in favor all "model processing engines"
// //       being built directly into the libapollo.so
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
//     lock_guard<mutex> lock(object_lock);
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


