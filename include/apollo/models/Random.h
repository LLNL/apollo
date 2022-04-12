// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include <memory>
#include <random>

#include "apollo/PolicyModel.h"

class Random : public PolicyModel
{
public:
  Random(int num_policies);
  ~Random();

  //
  int getIndex(std::vector<float> &features);
  void load(const std::string &filename){};
  void store(const std::string &filename){};
  bool isTrainable() { return false; }
  void train(Apollo::Dataset &dataset) {}

private:
  std::random_device random_dev;
  std::mt19937 random_gen;
  std::uniform_int_distribution<> random_dist;
};  // end: Apollo::Model::Random (class)


#endif
