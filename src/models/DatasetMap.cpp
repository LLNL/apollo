// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/DatasetMap.h"

#include <cstring>
#include <string>

namespace apollo
{

int DatasetMap::getIndex(std::vector<float> &features)
{
  if (best_policies.count(features) == 0) {
    std::cerr << "DatasetMap does not have an entry for those features\n";
    abort();
  }

  int policy = best_policies[features].first;

  return policy;
}

void DatasetMap::train(Apollo::Dataset &dataset)
{
  std::vector<std::vector<float>> features;
  std::vector<int> policies;
  dataset.findMinMetricPolicyByFeatures(features, policies, best_policies);
}

}  // end namespace apollo.