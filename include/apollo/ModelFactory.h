// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODEL_FACTORY_H
#define APOLLO_MODEL_FACTORY_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "apollo/PolicyModel.h"
#include "apollo/TimingModel.h"

namespace apollo
{
// Factory
class ModelFactory
{
public:
  static std::unique_ptr<PolicyModel> createPolicyModel(
      const std::string &model_name,
      int num_policies,
      const std::string &path);
  static std::unique_ptr<PolicyModel> createPolicyModel(
      const std::string &model_name,
      int num_features,
      int num_policies,
      std::unordered_map<std::string, std::string> &model_params);

  static std::unique_ptr<TimingModel> createRegressionTree(
      Apollo::Dataset &dataset);
};  // end: ModelFactory

}  // end namespace apollo.

#endif
