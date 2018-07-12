
#include <iostream>
#include <cstdint>
#include <cstring>
#include <typeinfo>

#include "apollo/Apollo.h"
#include "apollo/Region.h"
#include "apollo/Model.h"
#include "apollo/Feature.h"
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

    baseRegion = new Apollo::Region(this, "APOLLO_rootRegion", 0);
    baseRegion->caliSetString("APOLLO_BINDING_GUID", GLOBAL_BINDING_GUID);


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

void
Apollo::defineFeature(
        const char    *featureName,
        Apollo::Goal   goal,
        Apollo::Unit   unit,
        void          *obj_ptr)
{

    SOS_val_type sosType;
    switch (unit) {
        case Apollo::Unit::INTEGER: sosType = SOS_VAL_TYPE_INT;    break;
        case Apollo::Unit::DOUBLE:  sosType = SOS_VAL_TYPE_DOUBLE; break;
        case Apollo::Unit::CSTRING: sosType = SOS_VAL_TYPE_STRING; break;
    default:                        sosType = SOS_VAL_TYPE_INT;    break;
    }

    SOS_guid groupID = SOS_uid_next(sos->uid.my_guid_pool);

	int ft = to_underlying(Apollo::Hint::DEPENDANT);
	int op = to_underlying(goal);
	int du = to_underlying(unit);

    // NOTE: This allows data to be correlated between Apollo/SOSflow/Caliper
    //       sources, without needing to resort to time-windowing or other
    //       less efficient means.
    SOS_pack_related(pub, groupID, "GLOBAL_BINDING_GUID",
            SOS_VAL_TYPE_LONG, GLOBAL_BINDING_GUID);
    //
    SOS_pack_related(pub, groupID, "featureName", SOS_VAL_TYPE_STRING, featureName);
	SOS_pack_related(pub, groupID, "featureType", SOS_VAL_TYPE_INT, &ft);
	SOS_pack_related(pub, groupID, "targetOperation", SOS_VAL_TYPE_INT, &op);
	SOS_pack_related(pub, groupID, "dataUnit", SOS_VAL_TYPE_INT, &du);
	SOS_pack_related(pub, groupID, "featureValue", sosType, obj_ptr);

	// Features are only set from outside of loops,
	// publishing each time is not a performance hit
	// in the client.
	SOS_publish(pub);

    return;
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

bool Apollo::isOnline()
{
    return ynConnectedToSOS;
}



