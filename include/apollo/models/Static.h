#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Static : public Apollo::ModelObject {
    public:
        Static();
        ~Static();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                std::string model_definition);
        //
        int  getIndex(void);

    private:
        int policy_choice;

}; //end: Apollo::Model::Static (class)


#endif
