#include "apollo/ModelFactory.h"

#include "apollo/models/Static.h"
#include "apollo/models/Random.h"
#include "apollo/models/RoundRobin.h"
#include "apollo/models/DecisionTree.h"
#include "apollo/models/RegressionTree.h"

std::unique_ptr<PolicyModel> ModelFactory::createStatic(int num_policies, int policy_choice) {
    return std::make_unique<Static>( num_policies, policy_choice );
}

std::unique_ptr<PolicyModel> ModelFactory::createRandom(int num_policies) {
    return std::make_unique<Random>( num_policies );
}

std::unique_ptr<PolicyModel> ModelFactory::createRoundRobin(int num_policies) {
    return std::make_unique<RoundRobin>( num_policies );
}

std::unique_ptr<PolicyModel> ModelFactory::createDecisionTree(int num_policies,
        std::vector< std::vector<float> > &features,
        std::vector<int> &responses ) {
    return std::make_unique<DecisionTree>( num_policies, features, responses );
}

std::unique_ptr<TimingModel> ModelFactory::createRegressionTree(
        std::vector< std::vector<float> > &features,
        std::vector<float> &responses ) {
    return std::make_unique<RegressionTree>( features, responses );
}

