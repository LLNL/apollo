
#ifndef APOLLO_MODELWRAPPER_H
#define APOLLO_MODELWRAPPER_H

#include <mutex>
#include <memory>

#include "apollo/Apollo.h"
#include "apollo/Model.h"
#include "apollo/Region.h"

class Apollo::ModelWrapper {
    public:
        ModelWrapper(
                Apollo      *apollo,
                int          numPolicies);
        ~ModelWrapper();

        bool configure(Apollo::ModelCategory model_cat, const char *model_def);

    private:
        Apollo *apollo;
        //
        Apollo::Region *baseRegion;  //Lowest-level context.  (automatic)
        //
        std::string     id;
        int             num_policies;
        // 
        std::shared_ptr<Apollo::ModelObject> model_ptr;

        // ----------
        // NOTE: Deprecated because we're not dlopen'ing models
        //       as external shared objects.
        //       Keeping this as function signature reference.
        //Apollo::ModelObject* (*create)();
        // NOTE: Destruction is handled by std::shared_ptr now.
        //void (*destroy)(Apollo::ModelObject*);

}; //end: Apollo::Model


#endif

