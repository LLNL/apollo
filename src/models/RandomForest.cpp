// Copyright (c) 2021, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
// See the top-level LICENSE file for details
// SPDX-License-Identifier: MIT

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <sys/stat.h>

#include "apollo/models/RandomForest.h"
#include "models/impl/RandomForestImpl.h"

#ifdef ENABLE_OPENCV
#include <opencv2/core/types.hpp>
#endif

using namespace std;

static inline bool fileExists(std::string path) {
    struct stat stbuf;
    return (stat(path.c_str(), &stbuf) == 0);
}

RandomForest::RandomForest(int num_policies, std::string path)
    : PolicyModel(num_policies, "RandomForest", false)
{
  if (not fileExists(path)) {
    std::cerr << "== APOLLO: Cannot access the RandomForest model requested:\n"
              << "== APOLLO:     " << path << "\n"
              << "== APOLLO: Exiting.\n";
    abort();
  }
  // The file at least exists... attempt to load a model from it!
  std::cout << "== APOLLO: Loading the requested RandomForest:\n"
            << "== APOLLO:     " << path << "\n";
#ifdef ENABLE_OPENCV
  rfc = RTrees::load(path.c_str());
#else
  rfc = std::make_unique<RandomForestImpl>(num_policies, path);
#endif
  return;
}

RandomForest::RandomForest(int num_policies,
                           std::vector<std::vector<float> > &features,
                           std::vector<int> &responses,
                           unsigned num_trees,
                           unsigned max_depth)
    : PolicyModel(num_policies, "RandomForest", false)
{
#ifdef ENABLE_OPENCV
  rfc = RTrees::create();
  rfc->setTermCriteria(
      TermCriteria(TermCriteria::MAX_ITER + TermCriteria::EPS, num_trees, 0.01));

  rfc->setMaxDepth(max_depth);

  rfc->setMinSampleCount(1);
  rfc->setRegressionAccuracy(0);
  rfc->setUseSurrogates(false);
  rfc->setMaxCategories(policy_count);
  rfc->setCVFolds(0);
  rfc->setUse1SERule(false);
  rfc->setTruncatePrunedTree(false);
  rfc->setPriors(Mat());

  Mat fmat;
  for (auto &i : features) {
    Mat tmp(1, i.size(), CV_32F, &i[0]);
    fmat.push_back(tmp);
  }

  Mat rmat;
  Mat(fmat.rows, 1, CV_32S, &responses[0]).copyTo(rmat);

  rfc->train(fmat, ROW_SAMPLE, rmat);
#else
  rfc = std::make_unique<RandomForestImpl>(
      num_policies, features, responses, num_trees, max_depth);
#endif

  return;
}

RandomForest::~RandomForest() { return; }

int RandomForest::getIndex(std::vector<float> &features)
{
  return rfc->predict(features);
}

void RandomForest::store(const std::string &filename)
{
  rfc->save(filename);
}
