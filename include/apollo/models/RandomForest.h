#ifndef APOLLO_MODELS_RANDOMFOREST_H
#define APOLLO_MODELS_RANDOMFOREST_H

#include <string>
#include <vector>

#include "apollo/PolicyModel.h"

#ifdef ENABLE_OPENCV
#include <opencv2/ml.hpp>
using namespace cv;
using namespace cv::ml;
#endif

class RandomForestImpl;

class RandomForest : public PolicyModel
{
public:
  RandomForest(
      int num_policies,
      std::vector<std::tuple<std::vector<float>, int, double> > &measures,
      unsigned num_trees,
      unsigned max_depth);
  RandomForest(int num_policies, std::string path);

  ~RandomForest();

  int getIndex(void);
  int getIndex(std::vector<float> &features);
  void store(const std::string &filename);
  void load(const std::string &filename);

private:
#ifdef ENABLE_OPENCV
  Ptr<RTrees> rfc;
#else
  std::unique_ptr<RandomForestImpl> rfc;
#endif
};


#endif
