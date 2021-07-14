// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include "apollo/ModelFactory.h"

#include "apollo/models/Static.h"
#include "apollo/models/Random.h"
#include "apollo/models/RoundRobin.h"
#include "apollo/models/DecisionTree.h"
#include "apollo/models/RegressionTree.h"
#include "apollo/models/Optimal.h"

std::unique_ptr<PolicyModel> ModelFactory::createStatic(int num_policies, int policy_choice) {
    return std::make_unique<Static>( num_policies, policy_choice );
}

std::unique_ptr<PolicyModel> ModelFactory::createRandom(int num_policies) {
    return std::make_unique<Random>( num_policies );
}

std::unique_ptr<PolicyModel> ModelFactory::createRoundRobin(int num_policies) {
    return std::make_unique<RoundRobin>( num_policies );
}


std::unique_ptr<PolicyModel> ModelFactory::loadDecisionTree(int num_policies,
        std::string path) {
    return std::make_unique<DecisionTree>( num_policies, path );
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

std::unique_ptr<PolicyModel> ModelFactory::createOptimal(std::string file) {
  return std::make_unique<Optimal>(file);
}
