// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_DECISIONTREEIMPL_H
#define APOLLO_MODELS_DECISIONTREEIMPL_H

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "helpers/OutputFormatter.h"
#include "helpers/Parser.h"

class DecisionTreeImpl
{
public:
  DecisionTreeImpl(int num_classes, std::istream &is);
  DecisionTreeImpl(int num_classes, std::string filename);
  DecisionTreeImpl(int num_classes, unsigned max_depth);
  DecisionTreeImpl(int num_classes,
                   std::vector<std::vector<float>> &features,
                   std::vector<int> &responses,
                   unsigned max_depth);
  ~DecisionTreeImpl();

  void train(std::vector<std::vector<float>> &features,
             std::vector<int> &responses);
  void load(const std::string &filename);
  void save(const std::string &filename);
  int predict(const std::vector<float> &features);
  void output_tree(OutputFormatter &outfmt,
                   std::string key,
                   bool include_data = true);
  void print_tree();

private:
  struct Node {
    float gini;
    size_t num_samples;
    int predicted_class;
    int feature_idx;
    float threshold;
    Node *left;
    Node *right;
    std::vector<size_t> count_per_class;
    DecisionTreeImpl &DT;

    Node(DecisionTreeImpl &DT,
         float gini,
         size_t num_samples,
         int predicted_class,
         int feature_idx,
         float threshold,
         Node *left,
         Node *right,
         std::vector<size_t> &count_per_class);


    Node(DecisionTreeImpl &DT,
         std::vector<size_t> &count_per_class,
         float gini,
         size_t num_samples);
  };

  // Returns pair(counts per class, gini value)
  template <typename Iterator>
  void compute_gini(const Iterator &Begin,
                    const Iterator &End,
                    float &gini_score);

  template <typename Iterator>
  std::vector<size_t> get_count_per_class(const Iterator &Begin,
                                          const Iterator &End);

  int evaluate_tree(const Node &tree, const std::vector<float> &features);
  void compile_and_link_jit_evaluate_function();
  void generate_source(Node &node, OutputFormatter &source_code);
  int (*jit_evaluate_function)(const std::vector<float> &features);

  // Returns tuple(min gini, iterator to split, feature index of split)
  template <typename Iterator>
  std::tuple<float, Iterator, size_t, float> split(const Iterator &Begin,
                                                   const Iterator &End);


  template <typename Iterator>
  Node *build_tree(const Iterator &Begin,
                   const Iterator &End,
                   const size_t max_depth,
                   size_t depth);

  void output_node(OutputFormatter &outfmt, Node &tree, std::string key);
  std::vector<size_t> parse_count_per_class(Parser &parser);

  void parse_tree(std::istream &is);
  Node *parse_node(Parser &parser);
  void parse_data(Parser &parser);

  std::vector<std::pair<std::vector<float>, int>> data;
  std::set<int> classes;
  Node *root;
  unsigned num_features;
  unsigned max_depth;
  unsigned num_classes;
  unsigned unique_id;
  static int unique_counter;
  std::vector<Node *> tree_nodes;
};

#endif