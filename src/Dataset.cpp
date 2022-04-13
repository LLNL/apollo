// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/Dataset.h"

#include <regex>

size_t Apollo::Dataset::size() { return data.size(); }

void Apollo::Dataset::clear() { data.clear(); }

const std::vector<std::tuple<std::vector<float>, int, double>> Apollo::Dataset::
    toVectorOfTuples() const
{
  std::vector<std::tuple<std::vector<float>, int, double>> vector;
  for (const auto &d : data) {
    const auto &features = d.first.first;
    const auto &policy = d.first.second;
    const auto &metric = d.second;
    vector.push_back(std::make_tuple(features, policy, metric));
  }

  return vector;
}

void Apollo::Dataset::insert(const std::vector<float> &features,
                             int policy,
                             double metric)
{
  auto key = std::make_pair(features, policy);
  auto it = data.find(key);
  if (it == data.end())
    data[key] = metric;
  else
    // Exponential moving average (a=0.5) to update the metric.
    it->second = .5 * it->second + .5 * metric;
}

void Apollo::Dataset::insert(Apollo::Dataset &ds)
{
  for (auto &d : ds.data) {
    const auto &features = d.first.first;
    const auto &policy = d.first.second;
    const auto &metric = d.second;
    insert(features, policy, metric);
  }
}

void Apollo::Dataset::findMinMetricPolicyByFeatures(
    std::vector<std::vector<float>> &features,
    std::vector<int> &policies,
    std::map<std::vector<float>, std::pair<int, double>> &min_metric_policies)
    const
{
  std::map<std::pair<std::vector<float>, int>, std::vector<double>> groups;
  std::map<std::vector<float>, std::pair<int, double>> best_policies;

  // Group measures by <features, policy> to gather metrics in a vector.
  for (auto &d : data) {
    const auto &features = d.first.first;
    const auto &policy = d.first.second;
    const auto &metric = d.second;

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
    policies.push_back(policy);
  }

  min_metric_policies = std::move(best_policies);
}

void Apollo::Dataset::store(std::ostream &os)
{
  os << "data: {\n";
  int idx = 0;
  for (auto &d : data) {
    const auto &features = d.first.first;
    const auto &policy = d.first.second;
    const auto &metric = d.second;
    os << "\t" << idx << ": { features: [ ";
    for (auto &f : features)
      os << f << ",";
    os << " ], ";
    os << "policy: " << policy << ", ";
    os << "xtime: " << metric;
    os << " },\n";
    ++idx;
  }
  os << "}\n";
}

void Apollo::Dataset::load(std::istream &is)
{
  std::smatch m;

  for (std::string line; std::getline(is, line);) {
    // std::cout << "PARSE LINE: " << line << "\n";
    if (std::regex_match(line, std::regex("#.*")))
      continue;
    else if (std::regex_match(line, std::regex("data: \\{")))
      continue;
    else if (std::regex_match(line,
                              m,
                              std::regex("\\s+[0-9]+: \\{ features: \\[ (.*) "
                                         "\\], "
                                         "policy: ([0-9]+), xtime: (.*) "
                                         "\\},"))) {

      std::vector<float> features;
      int policy;
      double xtime;

      std::string feature_list(m[1]);
      std::regex regex("([+|-]?[0-9]+\\.[0-9]*|[+|-]?[0-9]+),");
      std::sregex_token_iterator i =
          std::sregex_token_iterator(feature_list.begin(),
                                     feature_list.end(),
                                     regex,
                                     /* first sub-match*/ 1);
      std::sregex_token_iterator end = std::sregex_token_iterator();
      for (; i != end; ++i)
        features.push_back(std::stof(*i));
      policy = std::stoi(m[2]);
      xtime = std::stof(m[3]);

      // std::cout << "features: [ ";
      // for(auto &f : features)
      //   std::cout << f << ", ";
      // std::cout << "]\n";
      // std::cout << "policy " << policy << "\n";
      // std::cout << "xtime " << xtime << "\n";
      insert(features, policy, xtime);
    } else if (std::regex_match(line, std::regex("\\s*\\}")))
      break;
    else
      throw std::runtime_error("Error parsing dataset during loading line: " +
                               line);
  }
}
