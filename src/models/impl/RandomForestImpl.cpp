// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "RandomForestImpl.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "DecisionTreeImpl.h"

RandomForestImpl::RandomForestImpl(int num_classes, std::string filename)
    : num_classes(num_classes)
{
  load(filename);
}

RandomForestImpl::RandomForestImpl(int num_classes,
                                   unsigned num_trees,
                                   unsigned max_depth)
    : num_classes(num_classes), num_trees(num_trees), max_depth(max_depth)
{
}

void RandomForestImpl::train(std::vector<std::vector<float>> &features,
                             std::vector<int> &responses)
{
  // Uniform random generator using Mersenne-Twister for randomness.
  std::mt19937 generator;
  std::uniform_int_distribution<size_t> uniform_dist(0, features.size() - 1);

  for (unsigned i = 0; i < num_trees; i++) {
    std::vector<std::vector<float>> sample_features;
    std::vector<int> sample_responses;
    // Random pick with replacement of features, responses
    for (size_t i = 0, end = features.size(); i < end; ++i) {
      int sample_idx = uniform_dist(generator);
      sample_features.push_back(features[sample_idx]);
      sample_responses.push_back(responses[sample_idx]);
    }

    rfc.push_back(std::make_unique<DecisionTreeImpl>(
        num_classes, sample_features, sample_responses, max_depth));
  }
}

RandomForestImpl::RandomForestImpl(int num_classes,
                                   std::vector<std::vector<float>> &features,
                                   std::vector<int> &responses,
                                   unsigned num_trees,
                                   unsigned max_depth)
    : num_classes(num_classes), num_trees(num_trees), max_depth(max_depth)
{
  // Uniform random generator using Mersenne-Twister for randomness.
  std::mt19937 generator;
  std::uniform_int_distribution<size_t> uniform_dist(0, features.size() - 1);

  for (unsigned i = 0; i < num_trees; i++) {
    std::vector<std::vector<float>> sample_features;
    std::vector<int> sample_responses;
    // Random pick with replacement of features, responses
    for (size_t i = 0, end = features.size(); i < end; ++i) {
      int sample_idx = uniform_dist(generator);
      sample_features.push_back(features[sample_idx]);
      sample_responses.push_back(responses[sample_idx]);
    }

    rfc.push_back(std::make_unique<DecisionTreeImpl>(
        num_classes, sample_features, sample_responses, max_depth));
  }
}

void RandomForestImpl::output_rfc(OutputFormatter &outfmt)
{
  outfmt << "rfc: {\n";
  ++outfmt;
  outfmt << "num_trees: " & num_trees & ",\n";
  outfmt << "max_depth: " & max_depth & ",\n";
  outfmt << "classes: [ ";
  for (unsigned i = 0; i < num_classes; ++i) {
    outfmt &i & ", ";
  }
  outfmt & "],\n";
  outfmt << "trees:" & " [\n";
  unsigned tree_idx = 0;
  for (auto &dtree : rfc) {
    ++outfmt;
    dtree->output_tree(outfmt, "tree");
    outfmt << ",\n";
    --outfmt;
    tree_idx++;
  }
  outfmt << "]\n";
  --outfmt;
  outfmt << "}\n";
}

void RandomForestImpl::save(const std::string &filename)
{
  std::ofstream ofs(filename, std::ofstream::out);
  OutputFormatter outfmt(ofs);
  outfmt << "# RandomForestImpl\n";
  output_rfc(outfmt);
  ofs.close();
}
template <typename T>
static void parseKeyVal(Parser &parser, const char *key, T &val)
{
  parser.getNextToken();
  parser.parseExpected(key);

  parser.getNextToken();
  parser.parse(val);
  parser.parseExpected(",");
};

void RandomForestImpl::parse_rfc(std::ifstream &ifs)
{
  Parser parser(ifs);

  parser.getNextToken();
  parser.parseExpected("rfc:");

  parser.getNextToken();
  parser.parseExpected("{");

  parseKeyVal(parser, "num_trees:", num_trees);
  parseKeyVal(parser, "max_depth:", max_depth);

  parser.getNextToken();
  parser.parseExpected("classes:");
  parser.getNextToken();
  parser.parseExpected("[");
  int class_idx = 0;
  while (!parser.getNextTokenEquals("],")) {
    parser.parse(class_idx);
    parser.parseExpected(",");
    ++class_idx;
  }
  if (class_idx != num_classes)
    throw std::runtime_error("Expected class_idx " + std::to_string(class_idx) +
                             " equal to constructor num_classes " +
                             std::to_string(num_classes));

  parser.getNextToken();
  parser.parseExpected("trees:");
  parser.getNextToken();
  parser.parseExpected("[");

  for (int i = 0; i < num_trees; ++i) {
    rfc.push_back(std::make_unique<DecisionTreeImpl>(num_classes, ifs));
    parser.getNextToken();
    parser.parseExpected(",");
  }
  parser.getNextToken();
  parser.parseExpected("]");

  parser.getNextToken();
  parser.parseExpected("}");
}

void RandomForestImpl::load(const std::string &filename)
{
  std::ifstream ifs(filename);
  if (!ifs) throw std::runtime_error("Error loading file: " + filename);
  parse_rfc(ifs);
  ifs.close();
}

int RandomForestImpl::predict(const std::vector<float> &features)
{
  // Predict by majority vote over all decision trees.
  int count_per_class[num_classes];
  for (size_t i = 0; i < num_classes; ++i)
    count_per_class[i] = 0;

  for (auto &dtree : rfc) {
    int dtree_prediction = dtree->predict(features);
    assert(0 <= dtree_prediction && dtree_prediction < num_classes);
    count_per_class[dtree_prediction]++;
  }

  int class_idx = 0;
  for (size_t i = 1; i < num_classes; ++i)
    if (count_per_class[i] > count_per_class[class_idx]) class_idx = i;

  return class_idx;
}

void RandomForestImpl::print_forest()
{
  OutputFormatter outfmt(std::cout);
  output_rfc(outfmt);
}
