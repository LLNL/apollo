
#include <iostream.h>

#include "Apollo.hpp"

#include "raja.h"
#include "sos.h"
#include "sos_types.h"

static SOS_feedback_handler_f
handleFeedback(int msg_type, int msg_size, void *data)
{
    switch (msg_type) {
        //
        case SOS_FEEDBACK_TYPE_QUERY:
            apollo_log(1, "Query results received.\n");
            break;
        //
        case SOS_FEEDBACK_TYPE_PAYLOAD:
            apollo_log(1, "Trigger payload received.\n");
            break;
    }


    return;
}

Apollo::Apollo()
{
    sos = NULL;
    pub = NULL;

    SOS_init(&sos, SOS_ROLE_CLIENT,
            SOS_RECEIVES_DIRECT_FEEDBACK, handleFeedback);

    if (sos == NULL) {
        fprintf(stderr, "APOLLO: Unable to communicate with the SOS daemon.\n");
        return;
    }

    SOS_pub_init(sos, &pub, "APOLLO", SOS_NATURE_SUPPORT_EXEC);

    if (pub == NULL) {
        fprintf(stderr, "APOLLO: Unable to create publication handle.\n");
        SOS_finalize(sos);
        sos = NULL;
        return;
    }

    apollo_log(0, "Initialized.  (GUID == " << sos->my_guid << ")\n");

    return;
}



Apollo::~Apollo()
{
    if (sos != NULL) {
        SOS_finalize(sos);
        sos = NULL;
    }
}
