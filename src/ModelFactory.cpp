// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/ModelFactory.h"

#include <cassert>
#include <iostream>

#include "apollo/models/DecisionTree.h"
#include "apollo/models/Random.h"
#include "apollo/models/RandomForest.h"
#include "apollo/models/RoundRobin.h"
#include "apollo/models/Static.h"
#ifdef ENABLE_OPENCV
#include "apollo/models/RegressionTree.h"
#endif
#include "apollo/models/Optimal.h"
#include "apollo/models/PolicyNet.h"

std::unique_ptr<PolicyModel> ModelFactory::createPolicyModel(
    const std::string &model_name,
    int num_policies,
    const std::string &path)
{
  if (model_name == "Static" || model_name == "Random" ||
      model_name == "RoundRobin") {
    throw std::runtime_error(
        "Static, Random, RoundRobin do not support loading model from file.");
  } else if (model_name == "Optimal") {
    return std::make_unique<Optimal>(path);
  } else if (model_name == "DecisionTree")
    return std::make_unique<DecisionTree>(num_policies, path);
  else if (model_name == "RandomForest") {
    return std::make_unique<RandomForest>(num_policies, path);
  } else if (model_name == "PolicyNet") {
    throw std::runtime_error("Not impl. yet");
  } else {
    std::cerr << "Invalid model " << model_name << std::endl;
    abort();
  }
}

std::unique_ptr<PolicyModel> ModelFactory::createPolicyModel(
    const std::string &model_name,
    int num_features,
    int num_policies,
    std::unordered_map<std::string, std::string> &model_params)
{
  if (model_name == "Static") {
    int policy = 0;
    auto it = model_params.find("policy");
    if (it != model_params.end()) policy = std::stoi(it->second);
    if (policy < 0 || policy >= num_policies)
      throw std::runtime_error("Invalid policy " + std::to_string(policy) + " for Static");
    return std::make_unique<Static>(num_policies, policy);
  } else if (model_name == "Random") {
    return std::make_unique<Random>(num_policies);
  } else if (model_name == "RoundRobin") {
    return std::make_unique<RoundRobin>(num_policies);
  }
  else if (model_name == "DecisionTree") {
    // Default max_depth
    unsigned max_depth = 2;
    auto it = model_params.find("max_depth");
    if (it != model_params.end()) max_depth = std::stoul(it->second);
    std::unique_ptr<PolicyModel> explorer;
    it = model_params.find("explore");
    // TODO: enable explore_model_params? unneeded for now.
    std::unordered_map<std::string, std::string> explore_model_params;
    if (it != model_params.end())
      explorer = createPolicyModel(it->second,
                                   num_features,
                                   num_policies,
                                   explore_model_params);
    else
      // Default explorer model is RoundRobin.
      explorer = createPolicyModel("RoundRobin",
                                   num_features,
                                   num_policies,
                                   explore_model_params);

    return std::make_unique<DecisionTree>(num_policies,
                                          max_depth,
                                          explorer);
  } else if (model_name == "RandomForest") {
    // Default num_trees, max_depth
    unsigned num_trees = 10;
    unsigned max_depth = 2;
    auto it = model_params.find("num_trees");
    if (it != model_params.end()) num_trees = std::stoul(it->second);
    it = model_params.find("max_depth");
    if (it != model_params.end()) max_depth = std::stoul(it->second);
    std::unique_ptr<PolicyModel> explorer;
    it = model_params.find("explore");
    // TODO: Should we enable explore_model_params? It is unneeded for now since
    // exploration models RoundRobin, Random currently have no params.
    std::unordered_map<std::string, std::string> explore_model_params;
    if (it != model_params.end())
      explorer = createPolicyModel(it->second,
                                   num_features,
                                   num_policies,
                                   explore_model_params);
    else
      // Default explorer model is RoundRobin.
      explorer = createPolicyModel("RoundRobin",
                                   num_features,
                                   num_policies,
                                   explore_model_params);

    return std::make_unique<RandomForest>(num_policies,
                                          num_trees,
                                          max_depth,
                                          explorer);
  } else if (model_name == "PolicyNet") {
    double lr = 1e-2;
    double beta = 0.5;
    double beta1 = 0.5;
    double beta2 = 0.9;
    double threshold = 0.0;
    auto it = model_params.find("lr");
    if (it != model_params.end()) lr = std::stod(it->second);
    it = model_params.find("beta");
    if (it != model_params.end()) beta = std::stod(it->second);
    it = model_params.find("beta1");
    if (it != model_params.end()) beta1 = std::stod(it->second);
    it = model_params.find("beta2");
    if (it != model_params.end()) beta2 = std::stod(it->second);
    it = model_params.find("threshold");
    if (it != model_params.end()) threshold = std::stod(it->second);

    return std::make_unique<PolicyNet>(
        num_policies, num_features, lr, beta, beta1, beta2, threshold);
  }
  else if (model_name == "Optimal") {
    return std::make_unique<Optimal>();
  }
  else {
    std::cerr << __FILE__ << ":" << __LINE__ << ":: Invalid model "
              << model_name << std::endl;
    abort();
  }
}

std::unique_ptr<TimingModel> ModelFactory::createRegressionTree(
    Apollo::Dataset &dataset)
{
#ifdef ENABLE_OPENCV
  return std::make_unique<RegressionTree>(dataset);
#else
  throw std::runtime_error("Regression trees require OpenCV");
#endif
}
