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
