
#ifndef APOLLO_MODELWRAPPER_H
#define APOLLO_MODELWRAPPER_H

#include <mutex>

#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"

class Apollo::ModelWrapper {
    public:
        ModelWrapper(
                Apollo      *apollo,
                int          numPolicies);
        
        ~ModelWrapper();

        bool loadModel(const char *path, const char *definition);

    private:
        Apollo *apollo;
        //
        Apollo::Region *baseRegion;  //Lowest-level context.  (automatic)
        //
        std::string     id;
        int             num_policies;
        //
        std::string     object_path;
        std::mutex      object_lock;
        bool            object_loaded = false;
        //
        Apollo::Model   *model;
        Apollo::Model* (*create)();
        void           (*destroy)(Apollo::Model*);
        //
}; //end: Apollo::Model


#endif

