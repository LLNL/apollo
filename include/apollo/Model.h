#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include "apollo/Apollo.h"

// NOTE: This header is used to define actual models that
//       can be built and "plugged in" to Apollo at runtime.
//       These concrete models are managed through an interface
//       provided by the Apollo::ModelWrapper class.

class Apollo::Model {
    public:
        Model();
        virtual ~Model();

        virtual void configure(
                Apollo     *apollo_ptr,
                int         numPolicies,
                const char *model_definition);
        virtual int  getIndex(void);

    private:
        Apollo    *apollo;
        //
        bool       configured = false;
        //
        uint64_t     id;
        int          policyCount;
        std::string *model_def;
        int          iterCount;

}; //end: Apollo::Model (class)


#endif
