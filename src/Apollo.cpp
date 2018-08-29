
#include <iostream>
#include <cstdint>
#include <cstring>
#include <typeinfo>

#include "apollo/Apollo.h"
#include "apollo/Region.h"
#include "apollo/ModelWrapper.h"
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

    baseRegion = new Apollo::Region(this, "APOLLO", 0);

    apollo_log(0, "Initialized.\n");

    return;
}

Apollo::~Apollo()
{
    if (sos != NULL) {
        SOS_finalize(sos);
        sos = NULL;
    }
    delete baseRegion;
}

Apollo::Feature
Apollo::defineFeature(
        const char    *featureName,
        Apollo::Goal   goal,
        Apollo::Unit   unit,
        void          *objPtr)
{
    return Apollo::Feature(
            this,
            featureName,
            Apollo::Hint::INDEPENDENT,
            goal,
            unit,
            objPtr);

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

void Apollo::publish()
{
    SOS_publish(pub);
}


bool Apollo::isOnline()
{
    return ynConnectedToSOS;
}



