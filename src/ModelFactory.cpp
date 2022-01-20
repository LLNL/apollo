// Copyright (c) 2019-2021, Lawrence Livermore National Security, LLC
// and other Apollo project developers.
// Produced at the Lawrence Livermore National Laboratory.
// See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <cassert>
#include <iostream>

#include "apollo/ModelFactory.h"
#include "apollo/models/Static.h"
#include "apollo/models/Random.h"
#include "apollo/models/RoundRobin.h"
#include "apollo/models/DecisionTree.h"
#include "apollo/models/RandomForest.h"
#ifdef ENABLE_OPENCV
#include "apollo/models/RegressionTree.h"
#endif
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

std::unique_ptr<PolicyModel> ModelFactory::createTuningModel(
    const std::string &model_name,
    int num_policies,
    const std::string &path)
{
    if (model_name == "DecisionTree")
        return std::make_unique<DecisionTree>(num_policies, path);
    else if (model_name == "RandomForest") {
      return std::make_unique<RandomForest>(num_policies, path);
    }
    else {
        std::cerr << "Invalid model " << model_name << std::endl;
        abort();
    }
}

std::unique_ptr<PolicyModel> ModelFactory::createTuningModel(
    const std::string &model_name,
    int num_policies,
    std::vector<std::vector<float> > &features,
    std::vector<int> &responses,
    std::unordered_map<std::string, std::string> &model_params)
{
  if (model_name == "DecisionTree") {
    // Default max_depth
    unsigned max_depth = 2;
    auto it = model_params.find("max_depth");
    if ( it != model_params.end())
      max_depth = std::stoul(it->second);

    return std::make_unique<DecisionTree>(num_policies,
                                          features,
                                          responses,
                                          max_depth);
  } else if (model_name == "RandomForest") {
    // Default num_trees, max_depth
    unsigned num_trees = 10;
    unsigned max_depth = 2;
    auto it = model_params.find("num_trees");
    if (it != model_params.end())
      num_trees = std::stoul(it->second);
    it = model_params.find("max_depth");
    if (it != model_params.end())
      max_depth = std::stoul(it->second);

    return std::make_unique<RandomForest>(num_policies,
                                          features,
                                          responses,
                                          num_trees,
                                          max_depth);
  } else {
    std::cerr << __FILE__ << ":" << __LINE__ << ":: Invalid model "
              << model_name << std::endl;
    abort();
  }
}

std::unique_ptr<TimingModel> ModelFactory::createRegressionTree(
    std::vector<std::vector<float> > &features,
    std::vector<float> &responses)
{
#ifdef ENABLE_OPENCV
  return std::make_unique<RegressionTree>(features, responses);
#else
  throw std::runtime_error("Regression trees require OpenCV");
  return nullptr;
#endif
}

std::unique_ptr<PolicyModel> ModelFactory::createOptimal(std::string file) {
  return std::make_unique<Optimal>(file);
}
