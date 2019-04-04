#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <typeinfo>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/Region.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Feature.h"
//
#include "util/Debug.h"
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
            //apollo_log(1, "Trigger payload received."
            //        "  (msg_size == %d, data == %s\n",
            //        msg_size, (char *) data);

            void *apollo_ref = SOS_reference_get(
                    (SOS_runtime *)sos_context,
                    "APOLLO_CONTEXT");

            //TODO: The *data field needs to be strncpy'ed into a new patch of clean memory
            //      because it is not getting null terminated and this is leading to parse
            //      errors.
            //NOTE: Trace the ownership of the string, make sure it gets copied in
            //      somewhere else so we can free it after this function returns.
            
            char *cleanstr = (char *) calloc(msg_size + 1, sizeof(char));
            strncpy(cleanstr, (const char *)data, msg_size);
            call_Apollo_attachModel((struct ApolloDec *)apollo_ref, (char *) cleanstr);
            free(cleanstr);
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
        apollo_log(0, "[ERROR] apollo->attachModel() called with a"
                    " NULL model definition. Doing nothing.");
        return;
    }

    if (strlen(def) < 1) {
        apollo_log(0, "[ERROR] apollo->attachModel() called with an"
                    " empty model definition. Doing nothing.");
        return;
    }

    //__apollo_DEBUG_string(def, 5);

    std::vector<std::string> region_names;
    json j = json::parse(std::string(def));
    if (j.find("region_names") != j.end()) {
        region_names = j["region_names"].get<std::vector<std::string>>();
    }
    
    for (auto it : regions) {
        Apollo::Region *region = it.second; 
        Apollo::ModelWrapper *model = region->getModel();
        //
        if (std::find(std::begin(region_names), std::end(region_names),
                    region->name) != std::end(region_names)) {
            model->configure(def);
        }
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
        fprintf(stderr, "== APOLLO: [WARNING] Unable to communicate"
                " with the SOS daemon.\n");
        return;
    }

    SOS_pub_init(sos, &pub, (char *)"APOLLO", SOS_NATURE_SUPPORT_EXEC);
    SOS_reference_set(sos, "APOLLO_PUB", (void *) pub); 

    if (pub == NULL) {
        fprintf(stderr, "== APOLLO: [WARNING] Unable to create"
                " publication handle.\n");
        if (sos != NULL) {
            SOS_finalize(sos);
        }
        sos = NULL;
        return;
    }

    // At this point we have a valid SOS runtime and pub handle.
    // NOTE: The assumption here is that there is 1:1 ratio of Apollo
    //       instances per process.
    SOS_reference_set(sos, "APOLLO_CONTEXT", (void *) this); 
    SOS_sense_register(sos, "APOLLO_MODELS"); 

    ynConnectedToSOS = true; 
    
    SOS_guid guid = SOS_uid_next(sos->uid.my_guid_pool);

    //baseRegion = new Apollo::Region(this, "APOLLO", 0);

    apollo_log(1, "Initialized.\n");

    return;
}

Apollo::~Apollo()
{
    if (sos != NULL) {
        SOS_finalize(sos);
        sos = NULL;
    }
    //delete baseRegion;
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

std::string
Apollo::uniqueRankIDText(void)
{
    std::stringstream ss_text;
    ss_text << "{";
    ss_text << "hostname: \"" << pub->node_id      << "\",";
    ss_text << "pid: \""      << pub->process_id   << "\",";
    ss_text << "mpi_rank: \"" << pub->comm_rank    << "\"";
    ss_text << "}";
    return ss_text.str();
}

int
Apollo::sosPack(const char *name, int val)
{
    return SOS_pack(pub, name, SOS_VAL_TYPE_INT, &val);
}

int
Apollo::sosPackRelated(long relation_id, const char *name, int val)
{
    return SOS_pack_related(pub, relation_id, name, SOS_VAL_TYPE_INT, &val);
}
 
void Apollo::sosPublish()
{
    if (isOnline()) {
        SOS_publish(pub);
    }
}


bool Apollo::isOnline()
{
    return ynConnectedToSOS;
}



