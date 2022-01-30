// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_REGRESSIONTREE_H
#define APOLLO_MODELS_REGRESSIONTREE_H

#include <opencv2/ml.hpp>
#include <string>
#include <vector>
using namespace cv;
using namespace cv::ml;

#include "apollo/TimingModel.h"

class RegressionTree : public TimingModel
{

public:
  RegressionTree(
      std::vector<std::tuple<std::vector<float>, int, double> > &measures);


  ~RegressionTree();

  double getTimePrediction(std::vector<float> &features);
  void store(const std::string &filename);

private:
  // Ptr<DTrees> dtree;
  Ptr<RTrees> dtree;
  // Ptr<KNearest> dtree;
  // Ptr<Boost> dtree;
  // Ptr<ANN_MLP> dtree;
  // Ptr<LogisticRegression> dtree;
};  // end: RegressionTree (class)


#endif
