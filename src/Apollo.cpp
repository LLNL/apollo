
#include <iostream>

//#include <RAJA/RAJA.hpp>
//#include <RAJA/pattern/nested.hpp>

#include "Apollo.h"

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
            apollo_log(1, "Query results received.  (msg_size == %d)\n", msg_size);
            break;
        //
        case SOS_FEEDBACK_TYPE_PAYLOAD:
            apollo_log(1, "Trigger payload received.  (msg_size == %d, data == %s\n",
                    msg_size, (char *) data);
            break;
    }


    return;
}


Apollo::Apollo()
{
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
