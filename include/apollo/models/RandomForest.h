// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_RANDOMFOREST_H
#define APOLLO_MODELS_RANDOMFOREST_H

#include <memory>
#include <string>
#include <vector>

#include "apollo/PolicyModel.h"

#ifdef ENABLE_OPENCV
#include <opencv2/ml.hpp>
using namespace cv;
using namespace cv::ml;
#endif

class RandomForestImpl;

namespace apollo
{

class RandomForest : public PolicyModel
{
public:
  RandomForest(int num_policies,
               unsigned num_trees,
               unsigned max_depth,
               std::unique_ptr<PolicyModel> &explorer);
  RandomForest(int num_policies, std::string path);

  ~RandomForest();

  int getIndex(void);
  int getIndex(std::vector<float> &features);
  void store(const std::string &filename);
  void load(const std::string &filename);
  bool isTrainable();
  void train(Apollo::Dataset &dataset);

private:
#ifdef ENABLE_OPENCV
  Ptr<RTrees> rfc;
#else
  std::unique_ptr<RandomForestImpl> rfc;
#endif
  bool trainable;
  std::unique_ptr<PolicyModel> explorer;
};

}  // end namespace apollo.

#endif
