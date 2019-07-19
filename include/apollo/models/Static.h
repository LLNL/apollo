#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include <string>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Static : public Apollo::ModelObject {
    public:
        Static();
        ~Static();

        void configure(int num_policies, json model_definition);
        //
        int  getIndex(void);

    private:
        int policy_choice;

}; //end: Apollo::Model::Static (class)


#endif
