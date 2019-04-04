
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
    Apollo::ModelWrapper *model = getModel();
    assert (model != NULL);

    int choice = model->requestPolicyIndex();
   
    if (choice != current_policy) {
        exec_count_current_policy = 1;
        current_policy = choice;
        ((note *)note_current_policy)->end();
        ((note *)note_current_policy)->begin(current_policy);
    } else {
        exec_count_current_policy++;
    }

    ((note *)note_exec_count_current_policy)->end();
    ((note *)note_exec_count_current_policy)->begin(exec_count_current_policy);

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

    note_region_name =
        (void *) new note("region_name", CALI_ATTR_ASVALUE);
    note_current_step =
        (void *) new note("current_step", CALI_ATTR_ASVALUE);
    note_current_policy =
        (void *) new note("current_policy", CALI_ATTR_ASVALUE);
    note_exec_count_total =
        (void *) new note("exec_count_total", CALI_ATTR_ASVALUE);
    note_exec_count_current_step =
        (void *) new note("exec_count_current_step", CALI_ATTR_ASVALUE);
    note_exec_count_current_policy =
        (void *) new note("exec_count_current_policy", CALI_ATTR_ASVALUE);

    ((note *)note_current_step)->begin(current_step);
    ((note *)note_current_policy)->begin(current_policy);
    ((note *)note_region_name)->begin(name);
    ((note *)note_exec_count_current_step)->begin(exec_count_current_step);
    ((note *)note_exec_count_current_policy)->begin(exec_count_current_policy);
    ((note *)note_exec_count_total)->begin(exec_count_total);

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

    note *nobj;
    nobj = (note *) note_region_name; delete nobj;
    nobj = (note *) note_current_step; delete nobj;
    nobj = (note *) note_current_policy; delete nobj;
    nobj = (note *) note_exec_count_total; delete nobj;
    nobj = (note *) note_exec_count_current_step; delete nobj;
    nobj = (note *) note_exec_count_current_policy; delete nobj;
    nobj = NULL;
    note_region_name = NULL;
    note_current_step = NULL;
    note_current_policy = NULL;
    note_exec_count_total = NULL;
    note_exec_count_current_step = NULL;
    note_exec_count_current_policy = NULL;

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

    ((note *)note_current_step)->begin(current_step);
    ((note *)note_current_policy)->begin(current_policy);
    ((note *)note_region_name)->begin(name);
    ((note *)note_exec_count_current_step)->begin(exec_count_current_step);
    ((note *)note_exec_count_current_policy)->begin(exec_count_current_policy);
    ((note *)note_exec_count_total)->begin(exec_count_total);

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

    ((note *)note_current_step)->end();
    ((note *)note_current_policy)->end();
    ((note *)note_region_name)->end();
    ((note *)note_exec_count_current_step)->end();
    ((note *)note_exec_count_current_policy)->end();
    ((note *)note_exec_count_total)->end();
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



