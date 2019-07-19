
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/ModelWrapper.h"
//
#include "apollo/models/Random.h"
#include "apollo/models/Sequential.h"
#include "apollo/models/Static.h"
#include "apollo/models/RoundRobin.h"
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
        const char           *model_def_cstr)
{
    Apollo::Model::Type MT;
    int model_type = -1;

    std::string model_def;

    std::stringstream model_buffer; // Used only if reading model from file.

    if (model_def_cstr == nullptr) {
        model_type = MT.Default;
    } else if (strlen(model_def_cstr) < 1) {
        model_type = MT.Default;
    }

    if (model_type == MT.Default) {
        if (getenv("APOLLO_INIT_MODEL") != NULL) {
            log("Using ${APOLLO_INIT_MODEL} for region initialization.");
            const char *model_path = getenv("APOLLO_INIT_MODEL");
            log("Loading default model from file: ", model_path);
            std::ifstream model_file (model_path, std::ifstream::in);
            if (model_file.fail()) {
                fprintf(stderr, "== APOLLO: Error loading file specified in"
                        " ${APOLLO_INIT_MODEL}:\n\t\t%s\n", model_path);
                fprintf(stderr, "== APOLLO: Using MT.DefaultConfigJSON value:\n%s\n",
                        MT.DefaultConfigJSON);
                fflush(stderr);
                model_def = MT.DefaultConfigJSON;
            } else {
                model_buffer << model_file.rdbuf();
                model_def = model_buffer.str();
            }
        } else {
            model_def = MT.DefaultConfigJSON;
            log("Using the default model for initialization.");
        }
    }

    log("Model definition:\n", model_def);

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

    log("Attempting to parse model definition...");

    // Validate and extract model components

    // == [type]
    // == [type][guid]
    // == [type][name]
    int model_errors = 0;
    if (j.find("type") == j.end()) {
        log("Invalid model_def: missing [type]");
        model_errors++;
    } else {
        log("\t[type]");
        if (j["type"].find("guid") == j["type"].end()) {
            log("Invalid model_def: missing [type][guid]");
            model_errors++;
        } else {
            m_type_guid = j["type"]["guid"].get<uint64_t>();
            log("\t[type][guid] = ", m_type_guid);
        }
        if (j["type"].find("name") == j["type"].end()) {
            log("Invalid model_def: missing [type][name]");
            model_errors++;
        } else {
            m_type_name = j["type"]["name"].get<string>();
            log("\t[type][name] = ", m_type_name);
        }
    }

    // == [region_names]
    log("\t[region_names]");
    if (j.find("region_names") == j.end()) {
        log("Invalid model_def: missing [region_names]");
        model_errors++;
    } else {
        m_region_names = j["region_names"].get<vector<string>>();
    }

    // == [features]
    // == [features][count]
    // == [features][names]
    if (j.find("features") == j.end()) {
        log("Invalid model_def: missing [features]");
        model_errors++;
    } else {
        log("\t[features]");
        if (j["features"].find("count") == j["features"].end()) {
            log("Invalid model_def: missing [features][count]");
            model_errors++;
        } else {
            m_feat_count = j["features"]["count"].get<int>();
            log("\t[features][count] = ", m_feat_count);
        }
        if (j["features"].find("names") == j["features"].end()) {
            log("Invalid model_def: missing [features][names]");
            model_errors++;
        } else {
            m_feat_names = j["features"]["names"].get<vector<string>>();
            std::string s;
            for (const auto &elem : m_feat_names) s += elem;
            log("\t[features][names] = ", s);
        }
    }

    // == [driver]
    // == [driver][format]
    // == [driver][rules]
    // == [driver][rules][*regname*] <-- Note the different behavior for initial models...
    if (j.find("driver") == j.end()) {
        log("Invalid model_def: missing [driver]");
        model_errors++;
    } else {
        log("\t[driver]");
        if (j["driver"].find("format") == j["driver"].end()) {
            log("Invalid model_def: missing [driver][format]");
            model_errors++;
        } else {
            m_drv_format = j["driver"]["format"].get<string>();
            log("\t[driver][format] = ", m_drv_format);
        }

        // TODO [optimize]: This section could be handled outside of the configure
        //                  method perhaps, to prevent multiple json deserializations
        //                  and double-scans for the __ANY_REGION__ model.
        //                  Let's look at that later though, it's still fairly fast
        //                  and doesn't happen that often over the course of a run.
        if (j["driver"].find("rules") == j["driver"].end()) {
            log("Invalid model_def: missing [driver][rules] section");
            model_errors++;
        } else {
            if (j["driver"]["rules"].find(region->name) == j["driver"]["rules"].end()) {
                // We didn't find a model for this region.  Look for the "__ANY_REGION__"
                // magic tag, if it exits, use that one, otherwise error.
                if (j["driver"]["rules"].find("__ANY_REGION__") == j["driver"]["rules"].end()) {
                    log("Invalid model_def: missing [driver][rules][region->name]"
                                  " for region->name == \"", region->name,
                                  "\" and model package contains no __ANY_REGION__"
                                  " default.");
                    model_errors++;
                    m_drv_rules = nullptr;
                } else {
                    // We found a generic __ANY_REGION__ model
                    m_drv_rules = j["driver"]["rules"]["__ANY_REGION__"];
                    log("\t[driver][rules][__ANY_REGION__] being applied to ", region->name);
                }
            } else {
                // Best case! We DID find a specific model for this region
                m_drv_rules = j["driver"]["rules"][region->name];
                log("\t[driver][rules][", region->name, "] being applied to ", region->name);
            }

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
    else if (m_type_name == "RoundRobin")   { model_type = MT.RoundRobin; }
    else if (m_type_name == "DecisionTree") { model_type = MT.DecisionTree; }
    else                                    { model_type = MT.Default; }

    if (model_type == MT.Default) { model_type = APOLLO_DEFAULT_MODEL_TYPE; }
    shared_ptr<Apollo::ModelObject> nm = nullptr;

    switch (model_type) {
        //
        case MT.Random:
            log("Applying new Apollo::Model::Random()");
            nm = make_shared<Apollo::Model::Random>();
            break;
        case MT.Sequential:
            log("Applying new Apollo::Model::Sequential()");
            nm = make_shared<Apollo::Model::Sequential>();
            break;
        case MT.Static:
            log("Applying new Apollo::Model::Static()");
            nm = make_shared<Apollo::Model::Static>();
            break;
        case MT.RoundRobin:
            log("Applying new Apollo::Model::RoundRobin()");
            nm = make_shared<Apollo::Model::RoundRobin>();
            break;
        case MT.DecisionTree:
            log("Applying new Apollo::Model::DecisionTree()");
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
            // Production models:
        case MT.Static:
        case MT.DecisionTree:
            lnm->training = false;
            break;
            // Training/searching/bootstrapping models:
        case MT.Random:
        case MT.Sequential:
        case MT.RoundRobin:
        default:
            lnm->training = true;
            break;
    }

    lnm->configure(num_policies, m_drv_rules);
    lnm->setGuid(m_type_guid);

    model_sptr.reset(); // Release ownership of the prior model's shared ptr
    model_sptr = nm;    // Make this new model available for use.

    return true;
}


Apollo::ModelWrapper::ModelWrapper(
        Apollo           *apollo_ptr,
        Apollo::Region   *region_ptr,
        int               numPolicies)
{
    apollo = apollo_ptr;
    region = region_ptr;
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
            log("[WARNING] requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default).");
            return 0;
        } else if (err_count == 10) {
            log("[WARNING] requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default) and suppressing"
                    " additional identical error messages.");
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


