// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#include <memory>
#include <string>
#include <vector>

#include "apollo/PolicyModel.h"

#ifdef ENABLE_OPENCV
#include <opencv2/ml.hpp>
using namespace cv;
using namespace cv::ml;
#endif
class DecisionTreeImpl;

class DecisionTree : public PolicyModel
{

public:
  DecisionTree(int num_policies,
               unsigned max_depth,
               std::unique_ptr<PolicyModel> &explorer);
  DecisionTree(int num_policies, std::string path);

  ~DecisionTree();

  int getIndex(void);
  int getIndex(std::vector<float> &features);
  void store(const std::string &filename);
  bool isTrainable();
  void load(const std::string &filename);
  void train(Apollo::Dataset &dataset);

private:
#ifdef ENABLE_OPENCV
  Ptr<DTrees> dtree;
#else
  std::unique_ptr<DecisionTreeImpl> dtree;
#endif
  bool trainable;
  std::unique_ptr<PolicyModel> explorer;
};


#endif
