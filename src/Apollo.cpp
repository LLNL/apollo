
#include <iostream>
#include <sstream>
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
handleFeedback(void *sos_context, int msg_type, int msg_size, void *data)
{
    SOS_msg_header header;
    int   offset = 0;
    char *tree;
    struct ApolloDec;

    switch (msg_type) {
        //
        case SOS_FEEDBACK_TYPE_QUERY:
            apollo_log(1, "Query results received."
                    "  (msg_size == %d)\n", msg_size);
            break;
        case SOS_FEEDBACK_TYPE_CACHE:
            apollo_log(1, "Cache results received."
                    "  (msg_size == %d)\n", msg_size);
            break;
        //
        case SOS_FEEDBACK_TYPE_PAYLOAD:
            apollo_log(1, "Trigger payload received."
                    "  (msg_size == %d, data == %s\n",
                    msg_size, (char *) data);

            void *apollo_ref = SOS_reference_get(
                    (SOS_runtime *)sos_context,
                    "APOLLO_CONTEXT");
            call_Apollo_attachModel((struct ApolloDec *)apollo_ref, (char *) data);
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
    int i;

    if (def == NULL) {
        apollo_log(0, "ERROR: apollo->attachModel() called with a"
                    " NULL model definition. Doing nothing.");
        return;
    }

    if (strlen(def) < 1) {
        apollo_log(0, "ERROR: apollo->attachModel() called with an"
                    " empty model definition. Doing nothing.");
        return;
    }
    
    typedef std::stringstream unpack_str;
    std::istringstream model_def_sstream(def);
    std::string        line;

    int model_type = 0;
    std::getline(model_def_sstream, line);
    unpack_str(line) >> model_type;

    //std::cout << "CLIENT received the following model definition:\n";
    //std::cout << def;
    
    for (auto it : regions) {
        Apollo::Region *region = it.second; 
        std::cout << region->name << "...\n";

        Apollo::ModelWrapper *model = region->getModel();
        model->configure(def);

        /// ---
    };

    //std::cout << "Done attempting to load new model.\n";
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

    // NOTE: The assumption here is that there is 1:1 ratio of Apollo
    //       instances per process.
    SOS_reference_set(sos, "APOLLO_CONTEXT", (void *) this); 
    SOS_sense_register(sos, "APOLLO_MODELS"); 

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



