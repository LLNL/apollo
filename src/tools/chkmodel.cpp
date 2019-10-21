
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


#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

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


int main(int argc, char **argv)
{
    Apollo::Model::Type MT;
    int model_type = MT.Default;

    std::stringstream model_buffer;

    char *region_name = NULL;

    string         model_def;
    uint64_t       m_type_guid;
    string         m_type_name;
    vector<string> m_region_names;
    int            m_feat_count;
    vector<string> m_feat_names;
    string         m_drv_format;
    json           m_drv_rules;

    if (argc > 1) {
        region_name = argv[1];
    } else {
        region_name = (char *) "EXAMPLE_REGION_NAME";
    }

    log("region_name == \"", region_name, "\"");

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
                log("model_buffer.str().length() == ", model_buffer.str().length());
                model_def = model_buffer.str();
            }
        } else {
            model_def = MT.DefaultConfigJSON;
            log("Using the default model for initialization.");
        }
    }

    // Extract the various common elements from the model definition
    // and provide them to the configure method, independent of whatever
    // further definitions are unique to that model.
    json j = json::parse(model_def);

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
            if (j["driver"]["rules"].find(region_name) == j["driver"]["rules"].end()) {
                // We didn't find a model for this region.  Look for the "__ANY_REGION__"
                // magic tag, if it exits, use that one, otherwise error.
                if (j["driver"]["rules"].find("__ANY_REGION__") == j["driver"]["rules"].end()) {
                    log("Invalid model_def: missing [driver][rules][region_name]"
                                  " for region->name == \"", region_name,
                                  "\" and model package contains no __ANY_REGION__"
                                  " default.");
                    model_errors++;
                } else {
                    // We found a generic __ANY_REGION__ model
                    m_drv_rules = j["driver"]["rules"]["__ANY_REGION__"].get<json>();
                    log("\t[driver][rules][__ANY_REGION__] = ", m_drv_rules);
                }
            } else {
                // Best case! We DID find a specific model for this region
                m_drv_rules = j["driver"]["rules"][region_name].get<json>();
                log("\t[driver][rules][", region_name, "] = ", m_drv_rules);
            }

        }
    }


    if (model_errors > 0) {
        fprintf(stderr, "== APOLLO: [ERROR] There were %d errors parsing"
                " the supplied model definition.\n", model_errors);
        exit(EXIT_FAILURE);
    } else {
        log("Imported model successfully!\n");
    }

    return EXIT_SUCCESS;
}




    //Code from the normal library...

    //if      (m_type_name == "Random")       { model_type = MT.Random; }
    //else if (m_type_name == "Sequential")   { model_type = MT.Sequential; }
    //else if (m_type_name == "Static")       { model_type = MT.Static; }
    //else if (m_type_name == "RoundRobin")   { model_type = MT.RoundRobin; }
    //else if (m_type_name == "DecisionTree") { model_type = MT.DecisionTree; }
    //else                                    { model_type = MT.Default; }

    //if (model_type == MT.Default) { model_type = APOLLO_DEFAULT_MODEL_TYPE; }
    //shared_ptr<Apollo::ModelObject> nm = nullptr;

    //switch (model_type) {
    //    //
    //    case MT.Random:
    //        log("Applying new Apollo::Model::Random()");
    //        nm = make_shared<Apollo::Model::Random>();
    //        break;
    //    case MT.Sequential:
    //        log("Applying new Apollo::Model::Sequential()");
    //        nm = make_shared<Apollo::Model::Sequential>();
    //        break;
    //    case MT.Static:
    //        log("Applying new Apollo::Model::Static()");
    //        nm = make_shared<Apollo::Model::Static>();
    //        break;
    //    case MT.RoundRobin:
    //        log("Applying new Apollo::Model::RoundRobin()");
    //        nm = make_shared<Apollo::Model::RoundRobin>();
    //        break;
    //    case MT.DecisionTree:
    //        log("Applying new Apollo::Model::DecisionTree()");
    //        nm = make_shared<Apollo::Model::DecisionTree>();
    //        break;
    //    //
    //    default:
    //         fprintf(stderr, "== APOLLO: [WARNING] Unsupported Apollo::Model::Type"
    //                 " specified to Apollo::ModelWrapper::configure."
    //                 " Doing nothing.\n");
    //         return false;
    //}

    //Apollo::ModelObject *lnm = nm.get();

    //switch (model_type) {
    //        // Production models:
    //    case MT.Static:
    //    case MT.DecisionTree:
    //        lnm->training = false;
    //        break;
    //        // Training/searching/bootstrapping models:
    //    case MT.Random:
    //    case MT.Sequential:
    //    case MT.RoundRobin:
    //    default:
    //        lnm->training = true;
    //        break;
    //}

    //lnm->configure(apollo, num_policies, m_drv_rules);
    //lnm->setGuid(m_type_guid);

    //model_sptr.reset(); // Release ownership of the prior model's shared ptr
    //model_sptr = nm;    // Make this new model available for use.


