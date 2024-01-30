// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_POLICY_MODEL_H
#define APOLLO_POLICY_MODEL_H

#include <string>
#include <vector>

#include "apollo/Dataset.h"


namespace apollo
{
// Abstract
class PolicyModel
{
public:
  PolicyModel(int num_policies, std::string name)
      : policy_count(num_policies), name(name){};
  virtual ~PolicyModel() {}
  //
  virtual int getIndex(std::vector<float> &features) = 0;

  virtual void store(const std::string &filename) = 0;
  virtual void load(const std::string &filename) = 0;

  virtual bool isTrainable() = 0;
  virtual void train(Apollo::Dataset &dataset) = 0;


  int policy_count;
  std::string name = "";
};  // end: PolicyModel (abstract class)

}  // end namespace apollo.

#endif
