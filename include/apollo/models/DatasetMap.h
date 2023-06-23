// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_STATICBEST_H
#define APOLLO_MODELS_STATICBEST_H

#include <map>
#include <string>

#include "apollo/PolicyModel.h"

namespace apollo
{
class DatasetMap : public PolicyModel
{
public:
  DatasetMap(int num_policies) : PolicyModel(num_policies, "DatasetMap"){};
  ~DatasetMap(){};

  //
  int getIndex(std::vector<float> &features);
  void load(const std::string &filename){};
  void store(const std::string &filename){};
  bool isTrainable() { return false; }
  void train(Apollo::Dataset &dataset);

private:
  // Maps features -> policy.
  std::map<std::vector<float>, std::pair<int, double>> best_policies;

};  // end: DatasetMap (class)

}  // end namespace apollo.

#endif
