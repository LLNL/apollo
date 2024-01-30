// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/DecisionTree.h"

#include <sys/stat.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "models/impl/DecisionTreeImpl.h"

#ifdef ENABLE_OPENCV
#include <opencv2/core/types.hpp>
#endif

namespace apollo
{

static inline bool fileExists(std::string path)
{
  struct stat stbuf;
  return (stat(path.c_str(), &stbuf) == 0);
}

DecisionTree::DecisionTree(int num_policies, std::string path)
    : PolicyModel(num_policies, "DecisionTree"), trainable(false)
{
  if (not fileExists(path)) {
    std::cerr << "== APOLLO: Cannot access the DecisionTree model requested:\n"
              << "== APOLLO:     " << path << "\n"
              << "== APOLLO: Exiting.\n";
    abort();
  }
  // The file at least exists... attempt to load a model from it!
  std::cout << "== APOLLO: Loading the requested DecisionTree:\n"
            << "== APOLLO:     " << path << "\n";
#ifdef ENABLE_OPENCV
  dtree = DTrees::load(path.c_str());
#else
  dtree = std::make_unique<DecisionTreeImpl>(num_policies, path);
#endif

  return;
}

DecisionTree::DecisionTree(int num_policies,
                           unsigned max_depth,
                           std::unique_ptr<PolicyModel> &explorer)
    : PolicyModel(num_policies, "DecisionTree"),
      trainable(true),
      explorer(std::move(explorer))
{
#ifdef ENABLE_OPENCV
  dtree = DTrees::create();

  dtree->setMaxDepth(max_depth);

  dtree->setMinSampleCount(1);
  dtree->setRegressionAccuracy(0);
  dtree->setUseSurrogates(false);
  dtree->setMaxCategories(policy_count);
  dtree->setCVFolds(0);
  dtree->setUse1SERule(false);
  dtree->setTruncatePrunedTree(false);
  dtree->setPriors(Mat());

#else
  dtree = std::make_unique<DecisionTreeImpl>(num_policies, max_depth);
#endif

  return;
}

DecisionTree::~DecisionTree() { return; }

bool DecisionTree::isTrainable() { return trainable; }

void DecisionTree::train(Apollo::Dataset &dataset)
{
  std::vector<std::vector<float>> features;
  std::vector<int> responses;
  std::map<std::vector<float>, std::pair<int, double>> min_metric_policies;
  dataset.findMinMetricPolicyByFeatures(features,
                                        responses,
                                        min_metric_policies);

#ifdef ENABLE_OPENCV
  Mat fmat;
  for (auto &i : features) {
    Mat tmp(1, i.size(), CV_32F, &i[0]);
    fmat.push_back(tmp);
  }

  Mat rmat;
  Mat(fmat.rows, 1, CV_32S, &responses[0]).copyTo(rmat);

  dtree->train(fmat, ROW_SAMPLE, rmat);
#else
  dtree->train(features, responses);
#endif

  trainable = false;
}

int DecisionTree::getIndex(std::vector<float> &features)
{
  if (!trainable) return dtree->predict(features);

  return explorer->getIndex(features);
}

void DecisionTree::load(const std::string &filename)
{
  trainable = false;
  dtree->load(filename);
}
void DecisionTree::store(const std::string &filename) { dtree->save(filename); }

}  // end namespace apollo.
