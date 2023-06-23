// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_DATASET_H
#define APOLLO_DATASET_H

#include <iostream>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

#include "apollo/Apollo.h"

class Apollo::Dataset
{
public:
  size_t size();
  void clear();

  void insert(std::vector<float> &features, int policy, double metric);
  void insert(Apollo::Dataset &ds);

  const std::vector<std::tuple<std::vector<float>, int, double>>
  toVectorOfTuples() const;

  void findMinMetricPolicyByFeatures(
      std::vector<std::vector<float>> &features,
      std::vector<int> &policies,
      std::map<std::vector<float>, std::pair<int, double>> &min_metric_policies)
      const;

  void load(std::istream &is);
  void store(std::ostream &os);

private:
  //  Key: features, policy -> value: metric (execution time) exponential moving
  //  average.
  std::map<std::pair<std::vector<float>, int>, double> data;
};  // end: Apollo::Dataset

#endif