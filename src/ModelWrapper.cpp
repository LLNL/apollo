
// Copyright (c) 2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


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
    } else {
        log("Using a learned model supplied to the .configure() method.");
        model_def = model_def_cstr;
    }

    log("Model definition is ", model_def.length(), " bytes long.\n");

    // Extract the various common elements from the model definition
    // and provide them to the configure method, independent of whatever
    // further definitions are unique to that model.
    json j = json::parse(model_def);

    uint64_t       m_guid;
    string         m_type_name;
    vector<string> m_region_names;
    int            m_feat_count;
    vector<string> m_feat_names;
    json           m_drv_rules;
    bool           m_timed;
    int            m_least_count;


    log("Attempting to parse model definition...");

    // Validate and extract model components
    int model_errors = 0;

    // == [guid]
    // == [type][name]
    if (j.find("guid") == j.end()) {
        log("Invalid model_def: missing [guid]");
        model_errors++;
    } else {
        m_guid = j["guid"].get<uint64_t>();
        log("\t[guid] = ", m_guid);
    }
    // == [region_names]
    log("\t[region_names]");
    if (j.find("region_names") == j.end()) {
        log("Invalid model_def: missing [region_names]");
        model_errors++;
    } else {
        m_region_names = j["region_names"].get<vector<string>>();
    }

    // == [region_types]
    log("\t[region_types]");
    if (j.find("region_types") == j.end()) {
        log("Invalid model_def: missing [region_types]");
        model_errors++;
    //} else {
    //    m_region_types = j["region_types"].get<vector<json>>();
    }

    // == [region_sizes]
    log("\t[region_sizes]");
    if (j.find("region_sizes") == j.end()) {
        log("Invalid model_def: missing [region_sizes]");
        model_errors++;
    //} else {
    //    m_region_sizes = j["region_sizes"].get<vector<json>>();
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
    // == [driver][rules]
    // == [driver][rules][*regname*]
    if (j.find("driver") == j.end()) {
        log("Invalid model_def: missing [driver]");
        model_errors++;
    } else {
        log("\t[driver]");
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
                    log("Using: MT.DefaultStaticModel == \"",
                            MT.DefaultStaticModel, "\"");
                    model_errors++;
                    m_drv_rules = json::parse(MT.DefaultStaticModel);
                    m_type_name = "Static";
                    m_timed = true;
                    m_least_count = -1;
                } else {
                    // We found a generic __ANY_REGION__ model
                    m_drv_rules = j["driver"]["rules"]["__ANY_REGION__"];
                    log("\t[driver][rules][__ANY_REGION__] being applied to ", region->name);
                    if (j["region_types"].find("__ANY_REGION__") == j["region_types"].end()) {
                        log("Invalid model_def: __ANY_REGION__ rule exists, but there is no type!");
                        log("Using: MT.DefaultStaticModel == \"",
                            MT.DefaultStaticModel, "\"");
                        model_errors++;
                        m_drv_rules = json::parse(MT.DefaultStaticModel);
                        m_type_name = "Static";
                        m_timed = true;
                        m_least_count = -1;
                    } else {
                        m_type_name = j["region_types"]["__ANY_REGION__"].get<string>();
                        m_timed = j["driver"]["timed"]["__ANY_REGION__"].get<bool>();
                        m_least_count = j["driver"]["least"]["__ANY_REGION__"].get<int>();
                    }
                }
            } else {
                // Best case! We DID find a specific model for this region
                m_drv_rules = j["driver"]["rules"][region->name];
                log("\t[driver][rules][", region->name, "] being applied to ", region->name);
                if (j["region_types"].find(region->name) == j["region_types"].end()) {
                    log("Invalid model_def: Region-specific rule exists, but there is no type!");
                    log("Using: MT.DefaultStaticModel == \"",
                            MT.DefaultStaticModel, "\"");
                    model_errors++;
                    m_drv_rules = json::parse(MT.DefaultStaticModel);
                    m_type_name = "Static";
                    m_timed = true;
                    m_least_count = -1;
                } else {
                    m_type_name = j["region_types"][region->name].get<string>();
                    m_timed = j["driver"]["timed"][region->name].get<bool>();
                    m_least_count = j["driver"]["least"][region->name].get<int>();
                }
            }
        }
    }

    region->is_timed = m_timed;
    region->minimum_elements_to_evaluate_model = m_least_count;

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
    lnm->setGuid(m_guid);

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


