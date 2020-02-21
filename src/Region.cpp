
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

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <limits>

#include "assert.h"

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Region.h"

int
Apollo::Region::getPolicyIndex(void)
{
    if ((not currently_inside_region)
        and (is_timed)){
        fprintf(stderr, "== APOLLO: [WARNING] region->getPolicyIndex() called"
                        " while NOT inside the region. Please call"
                        " region->begin(step) first so the model has values to use"
                        " when selecting a policy. (region->name == %s)\n", name);
        fflush(stderr);
    }

    //double evaluation_time_start;
    //double evaluation_time_stop;
    //double evaluation_time_total;
    //SOS_TIME(evaluation_time_start);

    Apollo::ModelWrapper *mw = getModelWrapper();
    assert (mw != NULL);

    int choice = mw->requestPolicyIndex();
    if (choice != current_policy) {
        apollo->setFeature("policy_index", (double) choice);
    }
    current_policy = choice;

    //SOS_TIME(evaluation_time_stop);
    //evaluation_time_total = evaluation_time_stop - evaluation_time_start;
    //log("getPolicyIndex took ", evaluation_time_total, " seconds.\n");

    return choice;
}


Apollo::Region::Region(
        Apollo      *apollo_ptr,
        const char  *regionName,
        int          numAvailablePolicies)
{
    apollo = apollo_ptr;
    name   = strdup(regionName);

    current_policy            = 0;
    currently_inside_region   = false;

    is_timed = true;
    minimum_elements_to_evaluate_model = -1;

    model_wrapper = new Apollo::ModelWrapper(apollo_ptr, this, numAvailablePolicies);
    model_wrapper->configure("");

    apollo->regions.insert({name, this});

    return;
}

Apollo::Region::~Region()
{
    if (currently_inside_region) {
        this->end();
    }

    if (name != NULL) {
        free(name);
        name = NULL;
    }

    return;
}


// NOTE: the parameter to begin() should be the time step
//       of the broader experimental context that this
//       region is being invoked to service. It may get
//       invoked several times for that time step, that is fine,
//       all invocations are automatically tracked internally.
//       If you do not know or have access to that time step,
//       passing in (and incrementing) a static int from the
//       calling context is typical.
void
Apollo::Region::begin(void) {
    if (is_timed == false) {
        return;
    }

    if (currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->begin() called"
                        " while already inside the region. Please call"
                        " region->end() first to avoid unintended"
                        " consequences. (region->name == %s)\n",
                        name);
        fflush(stderr);
    }
    currently_inside_region = true;

    SOS_TIME(current_exec_time_begin);

    // NOTE: Features are tracked globally within the process.
    //       Apollo semantics require that region.begin/end calls happen
    //       from the top-level process thread, they are not encountered
    //       within a parallel code region.
    //
    //       We update this value here in case another region used a different
    //       policy index, in case this value gets looked up between this
    //       call to region.begin and our model being newly evaluated by
    //       region.getPolicyIndex ...
    //
    apollo->setFeature("policy_index", (double) current_policy);


    return;
}

void
Apollo::Region::end(void) {
    if (is_timed == false) {
        return;
    }
    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->end() called"
                        " while NOT inside the region. Please call"
                        " region->begin(step) first to avoid unintended"
                        " consequences. (region->name == %s)\n", name);
        fflush(stderr);
    }

    currently_inside_region = false;

    SOS_TIME(current_exec_time_end);
    Apollo::Region::Measure *time = nullptr;

    // In case this was changed by the DecisionTree after the begin() call...
    apollo->setFeature("policy_index", (double) current_policy);

    auto iter = measures.find(apollo->features);
    if (iter == measures.end()) {
        time = new Apollo::Region::Measure;
        time->exec_count = 0;
        time->time_total = 0.0;
    } else {
        time = iter->second;
    }

    time->exec_count++;
    time->time_total += (current_exec_time_end - current_exec_time_begin);

    if (iter == measures.end()) {
        std::vector<Apollo::Feature> feat_copy = apollo->features;
        measures.insert({std::move(feat_copy), time});
    }

    if (false) {
        SOS_runtime *sos = (SOS_runtime *) apollo->sos_handle;
        int num_threads    = -1;
        int num_elements   = -1;
        int policy_index   = -1;
        for (Apollo::Feature ft : apollo->features)
        {
            if (     ft.name == "policy_index") { policy_index = (int) ft.value; }
            else if (ft.name == "num_threads")  { num_threads  = (int) ft.value; }
            else if (ft.name == "num_elements") { num_elements = (int) ft.value; }
        }
        std::cout.precision(17);
        std::cout \
            << "TRACE," \
            << current_exec_time_end << "," \
            << sos->config.node_id << ","
            << sos->config.comm_rank << ","
            << name << "," \
            << policy_index << "," \
            << num_threads << "," \
            << num_elements << "," \
            << std::fixed << (current_exec_time_end - current_exec_time_begin) \
            << std::endl;
    }

    return;
}


void
Apollo::Region::flushMeasurements(int assign_to_step) {
    SOS_runtime *sos = (SOS_runtime *) apollo->sos_handle;
    SOS_guid relation_id = 0;

    for (auto iter_measure = measures.begin();
             iter_measure != measures.end();   iter_measure++) {

        const std::vector<Apollo::Feature>& these_features = iter_measure->first;
        Apollo::Region::Measure                  *time_set = iter_measure->second;


        if (time_set->exec_count > 0) {
            relation_id = SOS_uid_next(sos->uid.my_guid_pool);

            for (Apollo::Feature ft : these_features) {
                apollo->sosPackRelatedDouble(relation_id, ft.name.c_str(), ft.value);
            }

            apollo->sosPackRelatedString(relation_id, "region_name", name);
            apollo->sosPackRelatedInt(relation_id, "step", assign_to_step);
            apollo->sosPackRelatedInt(relation_id, "exec_count", time_set->exec_count);
            apollo->sosPackRelatedDouble(relation_id, "time_avg",
                    (time_set->time_total / time_set->exec_count));

            if (false) {
                //----- exhaustive exploration report (begin)
                int num_threads    = -1;
                int num_elements   = -1;
                int policy_index   = -1;
                for (Apollo::Feature ft : these_features)
                {
                    if (     ft.name == "policy_index") {policy_index = (int)ft.value;}
                    else if (ft.name == "num_threads")  {num_threads  = (int)ft.value;}
                    else if (ft.name == "num_elements") {num_elements = (int)ft.value;}
                }
                std::cout.precision(17);
                std::cout \
                    << "REGION," \
                    << assign_to_step << "," \
                    << name << "," \
                    << time_set->exec_count << "," \
                    << policy_index << "," \
                    << num_threads << "," \
                    << num_elements << "," \
                    << std::fixed << (time_set->time_total / time_set->exec_count) \
                    << std::endl;
                //----- exhaustive exploration report (end)
            }

            time_set->exec_count = 0;
            time_set->time_total = 0.0;
        }
        // Note about optimization:
        //    We might delete these because things like "step" might be used as a
        //    feature, which means that time_set and the copy of the features vector
        //    used as a key to this measurement would never be revisited. For long
        //    running simulations that could lead to ugly memory leaks, esp.
        //    where there are very many features.
        //
        //    This overhead is required for Apollo to have any generality. If we wish to
        //    eliminate it, we're going to have to hardcode all of our features
        //    in the same way we've done the core ones like exec_count and time_total.
        //
        //
        //delete time_set;
        //measures.erase(iter_measure);
        //iter_measure = measures.begin();
    }


    return;
}



Apollo::ModelWrapper *
Apollo::Region::getModelWrapper(void) {
    return model_wrapper;
}



