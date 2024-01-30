// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_OPTIMAL_H
#define APOLLO_MODELS_OPTIMAL_H

#include <deque>
#include <string>

#include "apollo/PolicyModel.h"

namespace apollo
{

class Optimal : public PolicyModel
{
public:
  Optimal(std::string file);
  Optimal();
  ~Optimal(){};

  //
  int getIndex(std::vector<float> &features);
  void load(const std::string &filename);
  void store(const std::string &filename){};
  bool isTrainable() { return false; }
  void train(Apollo::Dataset &dataset) {}

private:
  std::deque<int> optimal_policy;

};  // end: Optimal (class)

}  // end namespace apollo.

#endif
