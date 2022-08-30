// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_RANDOMFORESTIMPL_H
#define APOLLO_MODELS_RANDOMFORESTIMPL_H

#include <memory>
#include <string>
#include <vector>

#include "DecisionTreeImpl.h"

class RandomForestImpl
{
public:
  RandomForestImpl(int num_classes, std::string filename);
  RandomForestImpl(int num_classes, unsigned num_trees, unsigned max_depth);
  RandomForestImpl(int num_classes,
                   std::vector<std::vector<float>> &features,
                   std::vector<int> &responses,
                   unsigned num_trees,
                   unsigned max_depth);

  void train(std::vector<std::vector<float>> &features,
             std::vector<int> &responses);
  void load(const std::string &filename);
  void save(const std::string &filename);
  int predict(const std::vector<float> &features);
  void print_forest();

private:
  void parse_rfc(std::ifstream &ifs);
  void output_rfc(OutputFormatter &outfmt);
  std::vector<std::unique_ptr<DecisionTreeImpl>> rfc;
  int num_classes;
  unsigned num_trees;
  unsigned max_depth;
};

#endif