#include <iostream>
#include <mutex>
#include <unordered_map>
#include <algorithm>

#include "assert.h"

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Region.h"

#include "caliper/cali.h"
#include "caliper/Annotation.h"

extern SOS_runtime *sos;
extern SOS_runtime *pub;

typedef cali::Annotation note;

    // NOTE: If we want to grab a GUID for some future annotation, this is how:
    // ----
    //SOS_guid guid = 0;
    //if (apollo->isOnline()) {
    //    guid = SOS_uid_next(sos->uid.my_guid_pool);
    //}

int
Apollo::Region::getPolicyIndex(void)
{
    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->getPolicyIndex() called"
                        " while NOT inside the region. Please call"
                        " region->begin(step) first so the model has values to use"
                        " when selecting a policy. (region->name == %s)\n", name);
        fflush(stderr);
    }

    Apollo::ModelWrapper *model = getModel();
    assert (model != NULL);

    int choice = model->requestPolicyIndex();

    if (choice != current_policy) {
        exec_count_current_policy = 1;
        current_policy = choice;
    } else {
        exec_count_current_policy++;
    }

    return choice;
}


Apollo::Region::Region(
        Apollo      *apollo_ptr,
        const char  *regionName,
        int          numAvailablePolicies)
{
    apollo = apollo_ptr;
    name   = strdup(regionName);

    current_step              = -1;
    current_policy            = -1;
    exec_count_total          = 0;
    exec_count_current_step   = 0;
    exec_count_current_policy = 0;
    currently_inside_region   = false;

    model = new Apollo::ModelWrapper(apollo_ptr, numAvailablePolicies);
    model->configure("");

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
Apollo::Region::begin(int for_experiment_time_step) {
    if (currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->begin(%d) called"
                        " while already inside the region. Please call"
                        " region->end() first to avoid unintended"
                        " consequences. (region->name == %s)\n",
                        current_step, name);
        fflush(stderr);
    }
    currently_inside_region = true;

    if (for_experiment_time_step != current_step) {
        exec_count_current_step = 0;
        current_step = for_experiment_time_step;
    }

    exec_count_total++;
    exec_count_current_step++;

    SOS_TIME(current_step_time_begin);

    return;
}

void
Apollo::Region::end(void) {
    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->end() called"
                        " while NOT inside the region. Please call"
                        " region->begin(step) first to avoid unintended"
                        " consequences. (region->name == %s)\n", name);
        fflush(stderr);
    }

    currently_inside_region = false;

    SOS_TIME(current_step_time_end);
    Apollo::Region::Measure *time = nullptr;

    apollo->setFeature("policy_index", (double) current_policy);

    auto iter = measures.find(apollo->features);
    if (iter == measures.end()) {
        time = new Apollo::Region::Measure;
        time->exec_count = 0;
        time->last = 0.0;
        time->min  = 9999999.99999;
        time->max  = 0.0;
        time->avg  = 0.0;
    } else {
        time = iter->second;
    }

    time->exec_count++;
    time->last = current_step_time_end - current_step_time_begin;
    time->min = std::min(time->min, time->last);
    time->max = std::max(time->max, time->last);
    time->avg -= (time->avg  / time->exec_count);   // simple rolling average.
    time->avg += (time->last / time->exec_count);   // (naively accumulates error)

    if (iter == measures.end()) {
        std::vector<Apollo::Feature> feat_copy = apollo->features;
        measures.insert({std::move(feat_copy), time});
    }

    return;
}


void
Apollo::Region::flushMeasurements(int assign_to_step) {
    note *t_flush =      (note *) Apollo::instance()->note_flush;
    note *t_for_region = (note *) Apollo::instance()->note_time_for_region;
    note *t_for_step   = (note *) Apollo::instance()->note_time_for_step;
    note *t_exec_count = (note *) Apollo::instance()->note_time_exec_count;
    note *t_last =       (note *) Apollo::instance()->note_time_last;
    note *t_min =        (note *) Apollo::instance()->note_time_min;
    note *t_max =        (note *) Apollo::instance()->note_time_max;
    note *t_avg =        (note *) Apollo::instance()->note_time_avg;

    for (auto iter_measure = measures.begin();
             iter_measure != measures.end();    ) {

        const std::vector<Apollo::Feature>& these_features = iter_measure->first;
        Apollo::Region::Measure                  *time_set = iter_measure->second;

        if (time_set->exec_count > 0) {
            t_flush->begin(0);
            t_for_region->begin(name);
            t_for_step->begin(assign_to_step);
            t_exec_count->begin(time_set->exec_count);
            t_last->begin(time_set->last);
            t_min->begin(time_set->min);
            t_max->begin(time_set->max);
            t_avg->begin(time_set->avg);

            for (Apollo::Feature ft : these_features) {
                apollo->noteBegin(ft.name, ft.value);
            }

            t_flush->end(); // Triggers the consumption of our annotation stack
            //              // by the Caliper service

            for (Apollo::Feature ft : these_features) {
                apollo->noteEnd(ft.name);
            }

            t_for_region->end();
            t_for_step->end();
            t_exec_count->end();
            t_last->end();
            t_min->end();
            t_max->end();
            t_avg->end();

        }

        delete time_set;
        measures.erase(iter_measure);
        iter_measure = measures.begin();
    }

    return;
}



Apollo::ModelWrapper *
Apollo::Region::getModel(void) {
    return model;
}






void
Apollo::Region::caliSetInt(const char *name, int value) {
    //fprintf(stderr, "== APOLLO: [WARNING] region->caliSetInt(%s, %d) called."
    //                " This function is likely to be deprecated due to"
    //                " the performance impacts of its frequent use.\n",
    //                name, value);
    //fflush(stderr);
    cali_set_int_byname(name, value);
    return;
}

void
Apollo::Region::caliSetString(const char *name, const char *value) {
    //fprintf(stderr, "== APOLLO: [WARNING] region->caliSetString(%s, %s) called."
    //                " This function is likely to be deprecated due to"
    //                " the performance impacts of its frequent use.\n",
    //                name, value);
    //fflush(stderr);
    cali_set_string_byname(name, value);
    return;
}


