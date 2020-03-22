#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include <random>
#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Random : public Apollo::Model {
    public:
        Random();
        ~Random();

        void configure(int num_policies);
        //
        int  getIndex(void);

    private:
}; //end: Apollo::Model::Random (class)


#endif
