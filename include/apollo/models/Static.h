// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include <string>

#include "apollo/PolicyModel.h"

class Static : public PolicyModel
{
public:
  Static(int num_policies, int policy_choice)
      : PolicyModel(num_policies, "Static," + std::to_string(policy_choice)),
        policy_choice(policy_choice){};
  ~Static(){};

  //
  int getIndex(std::vector<float> &features);
  void load(const std::string &filename){};
  void store(const std::string &filename){};
  bool isTrainable() { return false; }
  void train(Apollo::Dataset &dataset) {}

private:
  int policy_choice;

};  // end: Static (class)


#endif
