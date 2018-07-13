

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"
#include "apollo/Feature.h"

extern SOS_runtime *sos;
extern SOS_runtime *pub;

Apollo::Region::Region(
        Apollo      *apollo_ptr,
        const char  *regionName,
        int          numAvailablePolicies)
{
    apollo = apollo_ptr;
    name = strdup(regionName);

    cali_obj = NULL;
    cali_iter_obj = NULL;
    ynInsideMarkedRegion = false;

    model = new Apollo::Model(apollo_ptr, regionName);

    return;
}

void
Apollo::Region::handleCommonBeginTasks(void)
{
    if (ynInsideMarkedRegion == true) {
        // Free up the old region and make a new one,
        // to comply with Caliper "constructor == start"
        // conventions.
        //
        cali_obj->end();
        delete cali_obj;
        
    }
    SOS_guid guid = SOS_uid_next(sos->uid.my_guid_pool);
    snprintf(apollo->APOLLO_BINDING_GUID, 256, "%" SOS_GUID_FMT, guid);
    caliSetString("APOLLO_BINDING_GUID", apollo->APOLLO_BINDING_GUID);
    //
    cali_obj = new cali::Loop(name);

    ynInsideMarkedRegion = true;
    return;
}

void
Apollo::Region::handleCommonEndTasks(void)
{
    ynInsideMarkedRegion = false;

    if (cali_iter_obj != NULL) {
        delete cali_iter_obj;
        cali_iter_obj = NULL;
    }

    cali_obj->end();
    delete cali_obj;
    cali_obj = NULL;

    caliSetString("APOLLO_BINDING_GUID", apollo->APOLLO_BINDING_GUID);

    return;
}


void
Apollo::Region::begin(void) {
    handleCommonBeginTasks();
    
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
    if (ynInsideMarkedRegion == true) {
        if (cali_iter_obj != NULL) {
            delete cali_iter_obj;
        }
        cali_iter_obj = new cali::Loop::Iteration(
                cali_obj->iteration(static_cast<int>(i))  );
    } else {
        // Do nothing.  (There is nothing to iterate on.)
    } 

    return;  
}

void
Apollo::Region::iterationStop(void) {
    if (cali_iter_obj != NULL) {
        delete cali_iter_obj;
        cali_iter_obj = NULL;
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


Apollo::Model *
Apollo::Region::getModel(void) {
    return model;
}

Apollo::Region::~Region()
{
    free(name);
    return;
}



