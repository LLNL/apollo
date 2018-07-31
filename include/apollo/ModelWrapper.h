
#ifndef APOLLO_MODELWRAPPER_H
#define APOLLO_MODELWRAPPER_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"

class Apollo::ModelWrapper {
    public:
        ModelWrapper(
                Apollo      *apollo,
                const char  *path,
                int          numPolicies);
        
        ~ModelWrapper();

        bool loadModel(const char *path);
        int  requestPolicyIndex(void);

    private:
        Apollo *apollo;
        //
        Apollo::Region *baseRegion;  //Lowest-level context.  (automatic)
        //
        char   *modelID;
        char   *modelPath;   
        //
        Apollo::Model   *model;
        Apollo::Model* (*create)();
        void           (*destroy)(Apollo::Model*);
        //
        int     currentPolicyIndex;
}; //end: Apollo::Model


#endif

