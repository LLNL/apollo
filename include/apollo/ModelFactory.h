#ifndef APOLLO_MODEL_FACTORY_H
#define APOLLO_MODEL_FACTORY_H

#include <memory>
#include <vector>
#include "apollo/Model.h"

// Factory
class ModelFactory {
    public:
        static std::unique_ptr<Model> createStatic(int num_policies, int policy_choice );
        static std::unique_ptr<Model> createRandom(int num_policies);
        static std::unique_ptr<Model> createRoundRobin(int num_policies);
        static std::unique_ptr<Model> createDecisionTree(int num_policies,
                std::vector< std::vector<float> > &features,
                std::vector<int> &responses );
}; //end: ModelFactory


#endif
