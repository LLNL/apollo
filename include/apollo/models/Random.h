#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include <random>
#include <memory>

#include "apollo/PolicyModel.h"

class Random : public PolicyModel {
    public:
        Random(int num_policies);
        ~Random();

        //
        int  getIndex(std::vector<float> &features);

    private:
        int offset;

        std::random_device random_dev;
        std::mt19937 random_gen;
        std::uniform_int_distribution<> random_dist;
}; //end: Apollo::Model::Random (class)


#endif
