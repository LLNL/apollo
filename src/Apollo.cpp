
#include <iostream.h>

#include "Apollo.hpp"

#include "raja.h"
#include "sos.h"
#include "sos_types.h"

static SOS_feedback_handler_f
handleFeedback(int msg_type, int msg_size, void *data)
{

    return;
}

Apollo::Apollo()
{
    sos = NULL;
    pub = NULL;

    SOS_init(&sos, SOS_ROLE_CLIENT,
            SOS_RECEIVES_DIRECT_FEEDBACK, handleFeedback);

    if (sos == NULL) {
        std::cout << "APOLLO: Unable to communicate with the SOS daemon.\n";
        return;
    }

    SOS_pub_init(sos, &pub, "APOLLO", SOS_NATURE_SUPPORT_EXEC);

    if (pub == NULL) {
        std::cout << "APOLLO: Unable to create publication handle.\n";
        SOS_finalize(sos);
        sos = NULL;
        return;
    }

    if (APOLLO_VERBOSE) {
        std::cout << "APOLLO: Initialized.   "
            "(GUID == " << sos->my_guid << ")\n";
    }

    return;
}



Apollo::~Apollo()
{
    if (sos != NULL) {
        SOS_finalize(sos);
        sos = NULL;
    }
}
