#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

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
  DecisionTree(
      int num_policies,
      std::vector<std::tuple<std::vector<float>, int, double> > &measures,
      unsigned max_depth);
  DecisionTree(int num_policies, std::string path);

  ~DecisionTree();

  int getIndex(void);
  int getIndex(std::vector<float> &features);
  void store(const std::string &filename);
  void load(const std::string &filename);

private:
#ifdef ENABLE_OPENCV
  Ptr<DTrees> dtree;
#else
  std::unique_ptr<DecisionTreeImpl> dtree;
#endif
};


#endif
