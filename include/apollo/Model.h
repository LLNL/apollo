
#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include "apollo/Apollo.h"
#include "apollo/Region.h"

class Apollo::Model {
    public:
        Model(Apollo *apollo, const char *id);
        ~Model();

        int  requestPolicyIndex(void);

    private:
        Apollo *apollo;
        //
        Apollo::Region *baseRegion;  //Lowest-level context.  (automatic)
        //
        char   *modelID;
        char   *modelPattern;   // TODO: This will evolve.
        //
        int     currentPolicyIndex;
}; //end: Apollo::Model


#endif

