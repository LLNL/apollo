// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "Preprocessing.h"

#include <iostream>
#include <map>
#include <numeric>

void Preprocessing::findMinMetricPolicyByFeatures(
    const std::vector<std::tuple<std::vector<float>, int, double>> &measures,
    std::vector<std::vector<float>> &features,
    std::vector<int> &responses,
    std::map<std::vector<float>, std::pair<int, double>> &min_metric_policies)
{
  std::map<std::pair<std::vector<float>, int>, std::vector<double>> groups;
  std::map<std::vector<float>, std::pair<int, double>> best_policies;

  // Group measures by <features, policy> to gather metrics in a vector.
  for (auto &measure : measures) {
    const auto &features = std::get<0>(measure);
    const auto &policy = std::get<1>(measure);
    const auto &metric = std::get<2>(measure);

    auto pair = std::make_pair(features, policy);
    auto iter = groups.find({features, policy});
    if (iter == groups.end()) {
      std::vector<double> group_metrics = {metric};
      groups.insert(std::make_pair(pair, std::move(group_metrics)));
    } else
      groups[pair].push_back(metric);
  }

#ifdef DEBUG_OUTPUT
  for (auto &g : groups) {
    std::cout << "group [ ";
    for (auto &f : g.first.first)
      std::cout << f << ", ";
    std::cout << "] + " << g.first.second << " -> { ";
    for (auto &m : g.second)
      std::cout << m << ", ";
    std::cout << " }\n";
  }
#endif

  // Reduce grouped data to best_policies that minimize the metric.
  for (auto &g : groups) {
    const auto &features = g.first.first;
    const auto &policy = g.first.second;
    const auto &metrics = g.second;

    auto iter = best_policies.find(features);
    if (iter == best_policies.end()) {
      double avg =
          std::accumulate(metrics.begin(), metrics.end(), 0.0) / metrics.size();
      best_policies.insert(
          std::make_pair(features, std::make_pair(policy, avg)));
    } else {
      double avg = std::accumulate(g.second.begin(), g.second.end(), 0.0) /
                   g.second.size();
      double prev_avg = best_policies[features].second;
      if (avg < prev_avg) best_policies[features] = std::make_pair(policy, avg);
    }
  }

  for (auto &b : best_policies) {
#ifdef DEBUG_OUTPUT
    std::cout << "best [ ";
    for (auto &f : b.first)
      std::cout << f << ", ";
    std::cout << "] -> " << std::get<0>(b.second)
              << ", = " << std::get<1>(b.second) << "\n";
#endif

    const auto &train_features = b.first;
    const auto &policy = b.second.first;

    features.push_back(std::move(train_features));
    responses.push_back(policy);
  }

  min_metric_policies = std::move(best_policies);
}