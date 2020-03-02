
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


#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <unordered_map>
#include <algorithm>

#include <omp.h>

#include "CallpathRuntime.h"

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/Region.h"
#include "apollo/ModelWrapper.h"
//
#include "util/Debug.h"
//
#include "sos.h"
#include "sos_types.h"

void
handleFeedback(void *sos_context, int msg_type, int msg_size, void *data)
{
    Apollo *apollo = Apollo::instance();

    SOS_msg_header header;
    int   offset = 0;
    char *tree;
    struct ApolloDec;


    switch (msg_type) {
        //
        case SOS_FEEDBACK_TYPE_QUERY:
            log("Query results received. (msg_size == ", msg_size, ")");
            break;
        case SOS_FEEDBACK_TYPE_CACHE:
            log("Cache results received. (msg_size == ", msg_size, ")");
            break;
        //
        case SOS_FEEDBACK_TYPE_PAYLOAD:
            //NOTE: data may not be a null-terminated string, so we put it into one.
            char *cleanstr = (char *) calloc(msg_size + 1, sizeof(char));
            strncpy(cleanstr, (const char *)data, msg_size);
            call_Apollo_attachModel(apollo, (char *) cleanstr);
            free(cleanstr);
            break;
    }


    return;
}

extern "C" void
call_Apollo_attachModel(void *apollo_ref, const char *def)
{
    Apollo *apollo = (Apollo *) apollo_ref;
    apollo->attachModel(def);
    return;
}

void
Apollo::attachModel(const char *def)
{
    Apollo *apollo = Apollo::instance();

    int  i;
    bool def_has_wildcard_model = false;

    if (def == NULL) {
        log("[ERROR] apollo->attachModel() called with a"
                    " NULL model definition. Doing nothing.");
        return;
    }

    if (strlen(def) < 1) {
        log("[ERROR] apollo->attachModel() called with an"
                    " empty model definition. Doing nothing.");
        return;
    }

    // Extract the list of region names for this "package of models"
    std::vector<std::string> region_names;
    json j = json::parse(std::string(def));
    if (j.find("region_names") != j.end()) {
        region_names = j["region_names"].get<std::vector<std::string>>();
    }

    if (std::find(std::begin(region_names), std::end(region_names),
                "__ANY_REGION__") != std::end(region_names)) {
        def_has_wildcard_model = true;
    }

    // Roll through the regions in this process, and if it this region is
    // in the list of regions with a new model in the package, configure
    // that region's modelwrapper.  (The def* points to the whole package
    // of models, the configure() method will extract the specific model
    // that applies to it)
    for (auto it : regions) {
        Apollo::Region *region = it.second;
        if (def_has_wildcard_model) {
            // Everyone will get either a specific model from the definintion, or
            // the __ANY_REGION__ fallback.
            region->getModelWrapper()->configure(def);
            // TODO: We need a version that only attaches models to unassigned regions,
            // that does not update ALL regions, for instances where we have some regions
            // explicitly retraining and don't want to interrupt them
        } else {
            if (std::find(std::begin(region_names), std::end(region_names),
                        region->name) != std::end(region_names)) {
                region->getModelWrapper()->configure(def);
            }
        }
    };

    return;
}



Apollo::Apollo()
{
    ynConnectedToSOS = false;

    SOS_runtime *sos = NULL;
    SOS_pub     *pub = NULL;

    SOS_init(&sos, SOS_ROLE_CLIENT,
            SOS_RECEIVES_DIRECT_MESSAGES, handleFeedback);
    if (sos == NULL) {
        fprintf(stderr, "== APOLLO: [WARNING] Unable to communicate"
                " with the SOS daemon.\n");
        return;
    }

    SOS_pub_init(sos, &pub, (char *)"APOLLO", SOS_NATURE_SUPPORT_EXEC);
    SOS_reference_set(sos, "APOLLO_PUB", (void *) pub);

    if (pub == NULL) {
        fprintf(stderr, "== APOLLO: [WARNING] Unable to create"
                " publication handle.\n");
        if (sos != NULL) {
            SOS_finalize(sos);
        }
        sos = NULL;
        return;
    }

    // For other components of Apollo to access the SOS API w/out the include
    // file spreading SOS as a project build dependency, store these as void *
    // references in the class:
    sos_handle = sos;
    pub_handle = pub;

    log("Reading SLURM env...");
    try {
        numNodes = std::stoi(getenv("SLURM_NNODES"));
        log("    numNodes ................: ", numNodes);

        numProcs = std::stoi(getenv("SLURM_NPROCS"));
        log("    numProcs ................: ", numProcs);

        numCPUsOnNode = std::stoi(getenv("SLURM_CPUS_ON_NODE"));
        log("    numCPUsOnNode ...........: ", numCPUsOnNode);

        std::string envProcPerNode = getenv("SLURM_TASKS_PER_NODE");
        // Sometimes SLURM sets this to something like "4(x2)" and
        // all we care about here is the "4":
        auto pos = envProcPerNode.find('(');
        if (pos != envProcPerNode.npos) {
            numProcsPerNode = std::stoi(envProcPerNode.substr(0, pos));
        } else {
            numProcsPerNode = std::stoi(envProcPerNode);
        }
        log("    numProcsPerNode .........: ", numProcsPerNode);

        numThreadsPerProcCap = std::max(1, (int)(numCPUsOnNode / numProcsPerNode));
        log("    numThreadsPerProcCap ....: ", numThreadsPerProcCap);

    } catch (...) {
        fprintf(stderr, "== APOLLO: [ERROR] Unable to read values from SLURM"
                " environment variables.\n");
        if (sos != NULL) {
            SOS_finalize(sos);
        }
        exit(EXIT_FAILURE);
    }

    log("Reading OMP env...");
    ompDefaultSchedule   = omp_sched_static;       //<-- libgomp.so default
    ompDefaultNumThreads = numThreadsPerProcCap;   //<-- from SLURM calc above
    ompDefaultChunkSize  = -1;                     //<-- let OMP decide
    // Override the OMP defaults if there are environment variables set:
    char *val = NULL;
    val = getenv("OMP_NUM_THREADS");
    if (val != NULL) {
        ompDefaultNumThreads = std::stoi(val);
    }
    // We assume nonmonotinicity and chunk size of -1 for now.
    val = getenv("OMP_SCHEDULE");
    if (val != NULL) {
        std::string sched = getenv("OMP_SCHEDULE");
        if ((sched.find("static") != sched.npos)
            || (sched.find("STATIC") != sched.npos)) {
            ompDefaultSchedule = omp_sched_static;
        } else if ((sched.find("dynamic") != sched.npos)
            || (sched.find("DYNAMIC") != sched.npos)) {
            ompDefaultSchedule = omp_sched_dynamic;
        } else if ((sched.find("guided") != sched.npos)
            || (sched.find("GUIDED") != sched.npos)) {
            ompDefaultSchedule = omp_sched_guided;
        }
    }

    numThreads = ompDefaultNumThreads;

    // Explicitly set the current OMP defaults, so LLVM's OMP library doesn't
    // run really slow for no reason:
    //NOTE(chad): Deprecated.
    //omp_set_num_threads(ompDefaultNumThreads);
    //omp_set_schedule(ompDefaultSchedule, -1);




    // At this point we have a valid SOS runtime and pub handle.
    // NOTE: The assumption here is that there is 1:1 ratio of Apollo
    //       instances per process.
    SOS_reference_set(sos, "APOLLO_CONTEXT", (void *) this);
    SOS_sense_register(sos, "APOLLO_MODELS");

    ynConnectedToSOS = true;

    log("Initialized.");

    return;
}

Apollo::~Apollo()
{
    SOS_runtime *sos = (SOS_runtime *) sos_handle;
    if (sos != NULL) {
        SOS_finalize(sos);
        sos = NULL;
        sos_handle = NULL;
        // Leaving the pub in place leaks a little bit of memory, but
        // this destructor is only called when the process is terminating.
        pub_handle = NULL;
    }
}

void
Apollo::flushAllRegionMeasurements(int assign_to_step)
{
    SOS_pub *pub = (SOS_pub *) pub_handle;

    auto it = regions.begin();
    while (it != regions.end()) {
        Apollo::Region *reg = it->second;
        reg->flushMeasurements(assign_to_step);
        ++it;
    }
    SOS_publish(pub);
    return;
}


void
Apollo::setFeature(std::string set_name, double set_value)
{
    bool found = false;

    for (int i = 0; i < features.size(); ++i) {
        if (features[i].name == set_name) {
            found = true;
            features[i].value = set_value;
            break;
        }
    }

    if (not found) {
        Apollo::Feature f;
        f.name  = set_name;
        f.value = set_value;

        features.push_back(std::move(f));
    }

    return;
}

double
Apollo::getFeature(std::string req_name)
{
    double retval = 0.0;

    for(Apollo::Feature ft : features) {
        if (ft.name == req_name) {
            retval = ft.value;
            break;
        }
    };

    return retval;
}


Apollo::Region *
Apollo::region(const char *regionName)
{
    auto search = regions.find(regionName);
    if (search != regions.end()) {
        return search->second;
    } else {
        return NULL;
    }

}

std::string
Apollo::uniqueRankIDText(void)
{
    SOS_pub *pub = (SOS_pub *) pub_handle;
    std::stringstream ss_text;
    ss_text << "{";
    ss_text << "hostname: \"" << pub->node_id      << "\",";
    ss_text << "pid: \""      << pub->process_id   << "\",";
    ss_text << "mpi_rank: \"" << pub->comm_rank    << "\"";
    ss_text << "}";
    return ss_text.str();
}


int
Apollo::sosPackRelatedInt(uint64_t relation_id, const char *name, int val)
{
    if (isOnline()) {
        return SOS_pack_related((SOS_pub *)pub_handle, relation_id, name, SOS_VAL_TYPE_INT, &val);
    } else {
        return -1;
    }
}

int
Apollo::sosPackRelatedDouble(uint64_t relation_id, const char *name, double val)
{
    if (isOnline()) {
        return SOS_pack_related((SOS_pub *)pub_handle, relation_id, name, SOS_VAL_TYPE_DOUBLE, &val);
    } else {
        return -1;
    }
}

int
Apollo::sosPackRelatedString(uint64_t relation_id, const char *name, const char *val)
{
    if (isOnline()) {
        return SOS_pack_related((SOS_pub *)pub_handle, relation_id, name, SOS_VAL_TYPE_STRING, val);
    } else {
        return -1;
    }
}
int
Apollo::sosPackRelatedString(uint64_t relation_id, const char *name, std::string val)
{
    if (isOnline()) {
        return SOS_pack_related((SOS_pub *)pub_handle, relation_id, name, SOS_VAL_TYPE_STRING, val.c_str());
    } else {
        return -1;
    }
}

void Apollo::sosPublish()
{
    SOS_pub *pub = (SOS_pub *) pub_handle;
    if (isOnline()) {
        SOS_publish(pub);
    }
}


bool Apollo::isOnline()
{
    return ynConnectedToSOS;
}



