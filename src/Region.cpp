
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

typedef cali::Loop            loop;
typedef cali::Loop::Iteration iter;

int getApolloPolicyChoice(Apollo::Region *reg) 
{
    assert (reg != NULL); 
    Apollo::ModelWrapper *model = reg->getModel();
    assert (model != NULL);

    int choice = model->requestPolicyIndex();
    reg->caliSetInt("policyIndex", choice);

    return choice;
}


Apollo::Region::Region(
        Apollo      *apollo_ptr,
        const char  *regionName,
        int          numAvailablePolicies)
{
    apollo = apollo_ptr;
    name = strdup(regionName);

    apollo->regions.insert({name, this});

    cali_obj_ptr      = NULL;
    cali_iter_obj_ptr = NULL;
    ynInsideMarkedRegion = false;

    model = new Apollo::ModelWrapper(
            apollo_ptr,
            numAvailablePolicies);
    
    model->configure("");

    return;
}

Apollo::Region::~Region()
{
    if (name != NULL) {
        free(name);
    }
    return;
}


void
Apollo::Region::handleCommonBeginTasks(void)
{
   loop *cali_obj      = (loop *) cali_obj_ptr;
   iter *cali_iter_obj = (iter *) cali_iter_obj_ptr;
   if (ynInsideMarkedRegion == true) {
        // Free up the old region and make a new one,
        // to comply with Caliper "constructor == start"
        // conventions.
        //
        cali_obj->end();
        delete cali_obj;
        cali_obj_ptr = NULL;
        
    }

    SOS_guid guid = 0;
    if (apollo->isOnline()) {
        guid = SOS_uid_next(sos->uid.my_guid_pool);
    }
    //
    cali_obj_ptr = (void *) new cali::Loop(name);

    ynInsideMarkedRegion = true;
    return;
}

void
Apollo::Region::handleCommonEndTasks(void)
{
   loop *cali_obj      = (loop *) cali_obj_ptr;
   iter *cali_iter_obj = (iter *) cali_iter_obj_ptr;
    ynInsideMarkedRegion = false;

    if (cali_iter_obj != NULL) {
        delete cali_iter_obj;
        cali_iter_obj = NULL;
    }

    cali_obj = (loop *) cali_obj_ptr;

    cali_obj->end();
    delete cali_obj;
    cali_obj_ptr = NULL;

    return;
}


void
Apollo::Region::begin(void) {
    handleCommonBeginTasks();
   
    // NOTE: This is deprecated, features come directly through
    //       the Caliper SOS service.
    for(Apollo::Feature *feat : apollo->features) {
        //if (feat->getHint() != to_underlying(Apollo::Hint::DEPENDENT)) {
            feat->pack();
        //}
    }
    apollo->publish();
    return;
}

void
Apollo::Region::end(void)
{
    //
    // Log things here:
    
    //
    handleCommonEndTasks();
    return;
}

void
Apollo::Region::iterationStart(int i) {
    loop *cali_obj      = (loop *) cali_obj_ptr;
    iter *cali_iter_obj = (iter *) cali_iter_obj_ptr;
    if (ynInsideMarkedRegion == true) {
        if (cali_iter_obj_ptr != NULL) {
            delete cali_iter_obj;
            cali_iter_obj_ptr = NULL;
        }
        cali_iter_obj_ptr = (void *) new iter(
                cali_obj->iteration(static_cast<int>(i))  );
        cali_iter_obj = (iter *) cali_iter_obj_ptr;
        caliSetInt("iteration", i);
    } else {
        // Do nothing.  (There is nothing to iterate on.)
    } 

    return;  
}

void
Apollo::Region::iterationStop(void) {
    loop *cali_obj      = (loop *) cali_obj_ptr;
    iter *cali_iter_obj = (iter *) cali_iter_obj_ptr;
    if (cali_iter_obj != NULL) {
        delete cali_iter_obj;
        cali_iter_obj_ptr = NULL;
    }
    return;
}


void
Apollo::Region::caliSetInt(const char *name, int value) {
    cali_set_int_byname(name, value); 
    return;
}

void
Apollo::Region::caliSetString(const char *name, const char *value) {
    cali_set_string_byname(name, value); 
    return;
}


Apollo::ModelWrapper *
Apollo::Region::getModel(void) {
    return model;
}



