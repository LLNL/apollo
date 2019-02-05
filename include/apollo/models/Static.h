#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Static : public Apollo::ModelObject {
    public:
        Static();
        ~Static();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition);
        //
        int  getIndex(void);

    private:
        int policyChoice;

}; //end: Apollo::Model::Static (class)


#endif
