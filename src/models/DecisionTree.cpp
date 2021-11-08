// Copyright (c) 2019-2021, Lawrence Livermore National Security, LLC
// and other Apollo project developers.
// Produced at the Lawrence Livermore National Laboratory.
// See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include "apollo/models/DecisionTree.h"
#include "models/impl/DecisionTreeImpl.h"

#ifdef ENABLE_OPENCV
#include <opencv2/core/types.hpp>
#endif

static inline bool fileExists(std::string path) {
    struct stat stbuf;
    return (stat(path.c_str(), &stbuf) == 0);
}

DecisionTree::DecisionTree(int num_policies, std::string path)
    : PolicyModel(num_policies, "DecisionTree", false)
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
                           std::vector<std::vector<float> > &features,
                           std::vector<int> &responses,
                           unsigned max_depth)
    : PolicyModel(num_policies, "DecisionTree", false)
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

  Mat fmat;
  for (auto &i : features) {
    Mat tmp(1, i.size(), CV_32F, &i[0]);
    fmat.push_back(tmp);
  }

  Mat rmat;
  Mat(fmat.rows, 1, CV_32S, &responses[0]).copyTo(rmat);

  dtree->train(fmat, ROW_SAMPLE, rmat);
#else
  dtree = std::make_unique<DecisionTreeImpl>(num_policies, features, responses, max_depth);
#endif

  return;
}

DecisionTree::~DecisionTree() { return; }

int DecisionTree::getIndex(std::vector<float> &features)
{
  return dtree->predict(features);
}

void DecisionTree::store(const std::string &filename)
{
  dtree->save(filename);
}
