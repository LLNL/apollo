#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include "apollo/Model.h"

class Random : public Model {
    public:
        Random(int num_policies);
        ~Random();

        //
        int  getIndex(std::vector<float> &features);

    private:
}; //end: Apollo::Model::Random (class)


#endif
