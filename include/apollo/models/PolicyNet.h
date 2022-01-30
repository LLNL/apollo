// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_POLICYNET_H
#define APOLLO_MODELS_POLICYNET_H

#include <unordered_map>
#include <random>
#include <tuple>

#include "apollo/PolicyModel.h"

class Net;

class PolicyNet : public PolicyModel
{
public:
  PolicyNet(int num_policies,
            int num_features,
            double lr,
            double beta,
            double beta1,
            double beta2,
            double threshold);

  ~PolicyNet();

  int getIndex(std::vector<float> &features);

  void trainNet(std::vector<std::vector<float>> &states,
                std::vector<int> &actions,
                std::vector<double> &rewards);
  bool isTrainable() { return true; }
  void train(
      std::vector<std::tuple<std::vector<float>, int, double>> &measures);

  void store(const std::string &filename);
  void load(const std::string &filename);

  std::unordered_map<std::string, std::vector<double>> actionProbabilityMap;

private:
  int numPolicies;
  std::unique_ptr<Net> net;
  std::mt19937_64 gen;
  double rewardMovingAvg = 0;
  double beta;
  double threshold;
  int trainCount = 0;
};  // end: PolicyNet (class)


#endif
