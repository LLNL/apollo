#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include <random>
#include <string>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Random : public Apollo::ModelObject {
    public:
        Random();
        ~Random();

        void configure(int num_policies, json model_definition);
        //
        int  getIndex(void);

    private:
}; //end: Apollo::Model::Random (class)


#endif
