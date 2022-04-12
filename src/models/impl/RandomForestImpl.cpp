// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "RandomForestImpl.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <vector>

#include "DecisionTreeImpl.h"
#include "helpers/TimeTrace.h"

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
  unsigned tree_idx = 0;
  for (auto &dtree : rfc) {
    std::string key = std::string("tree_") + std::to_string(tree_idx);
    dtree->output_tree(outfmt, key);
    outfmt << ",\n";
    tree_idx++;
  }
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

void RandomForestImpl::parse_rfc(std::ifstream &ifs)
{
  std::smatch m;

  unsigned tree_idx = 0;
  for (std::string line; std::getline(ifs, line);) {
    // std::cout << "PARSE_RFC LINE: " << line << "\n";
    if (std::regex_match(line, std::regex("#.*")))
      continue;
    else if (std::regex_match(line, std::regex("rfc: \\{")))
      continue;
    else if (std::regex_match(line, std::regex("\\s+tree[_0-9]*: \\{"))) {
      if (num_trees <= tree_idx)
        throw std::runtime_error("Expected less trees");
      rfc.push_back(std::make_unique<DecisionTreeImpl>(num_classes, ifs));
      ++tree_idx;
    } else if (std::regex_match(line,
                                m,
                                std::regex("\\s+max_depth: ([0-9]+),")))
      max_depth = std::stoul(m[1]);
    else if (std::regex_match(line,
                              m,
                              std::regex("\\s+num_trees: ([0-9]+),"))) {
      num_trees = std::stoul(m[1]);
    } else if (std::regex_match(line, m, std::regex("\\s+classes: \\[.*\\],")))
      continue;
    else if (std::regex_match(line, std::regex("\\s+,")))
      continue;
    else if (std::regex_match(line, std::regex("\\}")))
      break;
    else
      throw std::runtime_error("Error parsing during loading line: " + line);
  }
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
