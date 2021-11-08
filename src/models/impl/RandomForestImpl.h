#ifndef APOLLO_MODELS_RANDOMFORESTIMPL_H
#define APOLLO_MODELS_RANDOMFORESTIMPL_H

#include "DecisionTreeImpl.h"

#include <string>
#include <vector>

class RandomForestImpl
{
public:
  RandomForestImpl(int num_classes, std::string filename);

  RandomForestImpl(int num_classes,
                   std::vector<std::vector<float>> &features,
                   std::vector<int> &responses,
                   unsigned num_trees,
                   unsigned max_depth);

  void save(const std::string &filename);
  int predict(const std::vector<float> &features);
  void print_forest();

private:
  void parse_rfc(std::ifstream &ifs);
  void load(const std::string &filename);
  void output_rfc(OutputFormatter &outfmt);
  std::vector<std::unique_ptr<DecisionTreeImpl>> rfc;
  int num_classes;
  unsigned num_trees;
  unsigned max_depth;
};

#endif