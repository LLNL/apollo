#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include <random>
#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Random : public Apollo::ModelObject {
    public:
        Random();
        ~Random();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                std::string model_definition);
        //
        int  getIndex(void);

    private:
}; //end: Apollo::Model::Random (class)


#endif
