// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_PREPROCESSING_H
#define APOLLO_MODELS_PREPROCESSING_H

#include <vector>
#include <tuple>
#include <map>

class Preprocessing
{
public:
  static void findMinMetricPolicyByFeatures(
      const std::vector<std::tuple<std::vector<float>, int, double>> &measures,
      std::vector<std::vector<float>> &features,
      std::vector<int> &responses,
      std::map<std::vector<float>, std::pair<int, double>> &min_metric_policies);
};

#endif
