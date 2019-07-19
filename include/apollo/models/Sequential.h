#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#include <string>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Sequential : public Apollo::ModelObject {
    public:
        Sequential();
        ~Sequential();

        void configure(int num_policies, json model_definition);
        //
        bool learning = true;
        int  getIndex(void);

    private:

}; //end: Apollo::Model::Sequential (class)


#endif
