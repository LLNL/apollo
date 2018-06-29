
#include <iostream>
#include <cstdint>
#include <string>
#include <typeinfo>

#include "Apollo.h"
//
#include "caliper/cali.h"
#include "caliper/Annotation.h"
//
#include "sos.h"
#include "sos_types.h"

SOS_runtime *sos;
SOS_pub     *pub;


void 
handleFeedback(int msg_type, int msg_size, void *data)
{

    switch (msg_type) {
        //
        case SOS_FEEDBACK_TYPE_QUERY:
            apollo_log(1, "Query results received."
                    "  (msg_size == %d)\n", msg_size);
            break;
        //
        case SOS_FEEDBACK_TYPE_PAYLOAD:
            apollo_log(1, "Trigger payload received."
                    "  (msg_size == %d, data == %s\n",
                    msg_size, (char *) data);
            break;
    }


    return;
}

Apollo::Apollo()
{
    ynConnectedToSOS = false;

    sos = NULL;
    pub = NULL;

    SOS_init(&sos, SOS_ROLE_CLIENT,
            SOS_RECEIVES_DIRECT_MESSAGES, handleFeedback);

    if (sos == NULL) {
        fprintf(stderr, "APOLLO: Unable to communicate with the SOS daemon.\n");
        return;
    }

    SOS_pub_init(sos, &pub, (char *)"APOLLO", SOS_NATURE_SUPPORT_EXEC);

    if (pub == NULL) {
        fprintf(stderr, "APOLLO: Unable to create publication handle.\n");
        SOS_finalize(sos);
        sos = NULL;
        return;
    }

    ynConnectedToSOS = true; 

    SOS_guid guid = SOS_uid_next(sos->uid.my_guid_pool);
    snprintf(GLOBAL_BINDING_GUID, 256, "%" SOS_GUID_FMT, guid);
    setNamedInt("APOLLO_BINDING_GUID", GLOBAL_BINDING_GUID);

    apollo_log(0, "Initialized.\n");

    return;
}



Apollo::~Apollo()
{
    if (sos != NULL) {
        SOS_finalize(sos);
        sos = NULL;
    }
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


Apollo::Region::Region(Apollo *apollo_ptr, const char *regionName)
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
    snprintf(CURRENT_BINDING_GUID, 256, "%" SOS_GUID_FMT, guid);
    caliSetString("APOLLO_BINDING_GUID", CURRENT_BINDING_GUID);
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

    caliSetString("APOLLO_BINDING_GUID", GLOBAL_BINDING_GUID);

    return;
}


void
Apollo::Region::begin(void) {
    handleCommonBeginTasks();
    // TODO: The default begin() behavior logs all features
    //       where f.hint != Feature::Hint.DEPENDANT

    return;
}

void
Apollo::Region::begin(std::list<Apollo::Feature *> logOnlyTheseFeatures)
{
    handleCommonBeginTasks();
    // TODO: Roll through the feature list and pack them.

    
    return;
}

void
Apollo::Region::begin(std::list<Apollo::Feature::Hint> logAnyMatchingHints)
{
    handleCommonBeginTasks();
    //TODO: Roll through the feature list and pack matching features.
    
    
    return;
}

void
Apollo::Region::end(void)
{
    // TODO: Pack all features where f.hint == Feature::Hint.DEPENDANT

    
    // ----
    handleCommonEndTasks();
    return;
}

void
Apollo::Region::end(std::list<Apollo::Feature *> logOnlyTheseSpecificFeatures)
{
    // TODO: Pack all features listed.
    

    // ----
    handleCommonEndTasks();
    return;
}

void
Apollo::Region::end(std::list<Apollo::Feature::Hint> logAnyMatchingTheseHints)
{
    // TODO: Pack all features matching the hints provided.
    

    // ----
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
Apollo::Region::caliSetString(const char *name, int value) {
    cali_set_string_byname(name, value); 
    return;
}

void
Apollo::Region::setFeature(
        const char               *featureName,
        Apollo::FeatureType       featureType,
        Apollo::TargetOperation   targetOperation,
        Apollo::DataUnit          dataUnit,
        void                     *value)
{

    SOS_val_type sosType;
    switch (dataUnit) {
    case DataUnit::INTEGER: sosType = SOS_VAL_TYPE_INT;    break;
    case DataUnit::DOUBLE:  sosType = SOS_VAL_TYPE_DOUBLE; break;
    case DataUnit::CSTRING: sosType = SOS_VAL_TYPE_STRING; break;
    default:                sosType = SOS_VAL_TYPE_INT;    break;
    }

    SOS_guid groupID = SOS_uid_next(sos->uid.my_guid_pool);

	int ft = to_underlying(featureType);
	int op = to_underlying(targetOperation);
	int du = to_underlying(dataUnit);

    // NOTE: This allows data to be correlated between Apollo/SOSflow/Caliper
    //       sources, without needing to resort to time-windowing or other
    //       less efficient means.
    SOS_pack_related(pub, groupID, "__APOLLO_BINDING_GUID",
            SOS_VAL_TYPE_LONG, current_binding_guid);
    //
    SOS_pack_related(pub, groupID, "featureName", SOS_VAL_TYPE_STRING, featureName);
	SOS_pack_related(pub, groupID, "featureType", SOS_VAL_TYPE_INT, &ft);
	SOS_pack_related(pub, groupID, "targetOperation", SOS_VAL_TYPE_INT, &op);
	SOS_pack_related(pub, groupID, "dataUnit", SOS_VAL_TYPE_INT, &du);
	SOS_pack_related(pub, groupID, "featureValue", sosType, value);

	// Features are only set from outside of loops,
	// publishing each time is not a performance hit
	// in the client.
	SOS_publish(pub);

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


Apollo::Model::Model(Apollo *apollo_ptr, const char *id_str) {
    apollo = apollo_ptr;
    model_id = strdup(id_str);
    // TODO: Map of regionNames to model_id in the Apollo class.
    //
    model_pattern = strdup("TODO: This is were the learned model will go.");
    current_policy_index = 0;
    return;
}

int
Apollo::Model::requestPolicyIndex(void) {
    // TODO: Interact with SOS to find out what to do.

    int choice = current_policy_index;

    current_policy_index++;
    if (current_policy_index > 5) {
        current_policy_index = 0;
    }

    return choice;
}

Apollo::Model::~Model() {
    free(model_id);
    free(model_pattern);
}

bool Apollo::isOnline()
{
    return ynConnectedToSOS;
}



