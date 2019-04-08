#include <iostream>
#include <mutex>

#include "assert.h"

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Region.h"
#include "apollo/Feature.h"

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

    policyTimers.resize(numAvailablePolicies);
    for (auto && t: policyTimers) {
        t.exec_count = 0;
        t.max  = 0.0;
        t.avg  = 0.0;
        t.last = 0.0;
        t.min  = 9999999.99999;
    }

    current_step              = -1;
    current_policy            = -1;
    exec_count_total          = 0;
    exec_count_current_step   = 0;
    exec_count_current_policy = 0;
    currently_inside_region   = false;

    // note_region_name =
    //     (void *) new note("region_name", CALI_ATTR_ASVALUE);
    // note_current_step =
    //     (void *) new note("current_step", CALI_ATTR_ASVALUE);
    // note_current_policy =
    //     (void *) new note("current_policy", CALI_ATTR_ASVALUE);
    // note_exec_count_total =
    //     (void *) new note("exec_count_total", CALI_ATTR_ASVALUE);
    // note_exec_count_current_step =
    //     (void *) new note("exec_count_current_step", CALI_ATTR_ASVALUE);
    // note_exec_count_current_policy =
    //     (void *) new note("exec_count_current_policy", CALI_ATTR_ASVALUE);

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

    // note *nobj;
    // nobj = (note *) note_region_name; delete nobj;
    // nobj = (note *) note_current_step; delete nobj;
    // nobj = (note *) note_current_policy; delete nobj;
    // nobj = (note *) note_exec_count_total; delete nobj;
    // nobj = (note *) note_exec_count_current_step; delete nobj;
    // nobj = (note *) note_exec_count_current_policy; delete nobj;
    // nobj = NULL;
    // note_region_name = NULL;
    // note_current_step = NULL;
    // note_current_policy = NULL;
    // note_exec_count_total = NULL;
    // note_exec_count_current_step = NULL;
    // note_exec_count_current_policy = NULL;

    return;
}




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

    // ((note *)note_current_step)->begin(current_step);
    // ((note *)note_current_policy)->begin(current_policy);
    // ((note *)note_region_name)->begin(name);
    // ((note *)note_exec_count_current_step)->begin(exec_count_current_step);
    // ((note *)note_exec_count_current_policy)->begin(exec_count_current_policy);
    // ((note *)note_exec_count_total)->begin(exec_count_total);

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
    auto *time = &policyTimers[current_policy];
    time->exec_count++;
    time->last = current_step_time_end - current_step_time_begin;
    time->min = std::min(time->min, time->last);
    time->max = std::max(time->max, time->last);
    time->avg -= (time->avg  / time->exec_count);   // simple rolling average.
    time->avg += (time->last / time->exec_count);   // (naively accumulates error)

    // ((note *)note_current_step)->end();
    // ((note *)note_current_policy)->end();
    // ((note *)note_region_name)->end();
    // ((note *)note_exec_count_current_step)->end();
    // ((note *)note_exec_count_current_policy)->end();
    // ((note *)note_exec_count_total)->end();

    return;
}


void
Apollo::Region::flushMeasurements(int assign_to_step) {
    note *t_flush =      (note *) Apollo::instance()->note_time_flush;
    note *t_for_region = (note *) Apollo::instance()->note_time_for_region;
    note *t_for_policy = (note *) Apollo::instance()->note_time_for_policy;
    note *t_for_step   = (note *) Apollo::instance()->note_time_for_step;
    note *t_exec_count = (note *) Apollo::instance()->note_time_exec_count;
    note *t_last =       (note *) Apollo::instance()->note_time_last;
    note *t_min =        (note *) Apollo::instance()->note_time_min;
    note *t_max =        (note *) Apollo::instance()->note_time_max;
    note *t_avg =        (note *) Apollo::instance()->note_time_avg;

    int pol_max = getModel()->getPolicyCount();

    for (int pol_idx = 0; pol_idx < pol_max; pol_idx++) {
        auto && t = policyTimers[pol_idx];

        if (t.exec_count > 0) {
            t_flush->begin(0);
            t_for_region->begin(name);
            t_for_policy->begin(pol_idx);
            t_for_step->begin(assign_to_step);
            t_exec_count->begin(t.exec_count);
            t_last->begin(t.last);
            t_min->begin(t.min);
            t_max->begin(t.max);
            t_avg->begin(t.avg);

            t_flush->end(); // Triggers the consumption of our annotation stack
            //              // by the Caliper service
            t_for_region->end();
            t_for_policy->end();
            t_for_step->end();
            t_exec_count->end();
            t_last->end();
            t_min->end();
            t_max->end();
            t_avg->end();
        
            // Clear out the summary now that we've flushed it, that way
            // this will only generate new traffic if this policy of this
            // region gets executed at least once, in the future:
            t.exec_count = 0;
            t.max  = 0.0;
            t.avg  = 0.0;
            t.last = 0.0;
            t.min  = 9999999.99999;
        }
    }

    return;
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


Apollo::ModelWrapper *
Apollo::Region::getModel(void) {
    return model;
}



