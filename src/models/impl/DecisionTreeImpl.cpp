// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "DecisionTreeImpl.h"

#include <dlfcn.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "helpers/Parser.h"

int DecisionTreeImpl::unique_counter = 0;

DecisionTreeImpl::DecisionTreeImpl(int num_classes, std::istream &is)
    : num_classes(num_classes)
{
  ++unique_counter;
  unique_id = unique_counter;
  parse_tree(is);
#ifdef ENABLE_JIT_DTREE
  compile_and_link_jit_evaluate_function();
#endif
}

DecisionTreeImpl::DecisionTreeImpl(int num_classes, std::string filename)
    : num_classes(num_classes)
{
  ++unique_counter;
  unique_id = unique_counter;
  load(filename);
#ifdef ENABLE_JIT_DTREE
  compile_and_link_jit_evaluate_function();
#endif
}

DecisionTreeImpl::DecisionTreeImpl(int num_classes, unsigned max_depth)
    : num_classes(num_classes), max_depth(max_depth)
{
  ++unique_counter;
  unique_id = unique_counter;
}
void DecisionTreeImpl::train(std::vector<std::vector<float>> &features,
                             std::vector<int> &responses)
{
  // Assumes all feature vectors are of equal size.
  num_features = features.begin()->size();
  classes.insert(responses.begin(), responses.end());
  data.clear();
  for (size_t i = 0, end = features.size(); i < end; ++i)
    data.emplace_back(std::move(features[i]), responses[i]);

  root = build_tree(data.begin(),
                    data.end(),
                    /* max_depth */ max_depth,
                    /* depth */ 0);

#ifdef ENABLE_JIT_DTREE
  compile_and_link_jit_evaluate_function();
#endif
}

DecisionTreeImpl::DecisionTreeImpl(int num_classes,
                                   std::vector<std::vector<float>> &features,
                                   std::vector<int> &responses,
                                   unsigned max_depth)
    : num_classes(num_classes), max_depth(max_depth)
{
  ++unique_counter;
  unique_id = unique_counter;
  // Assumes all feature vectors are equal.
  num_features = features.begin()->size();
  classes.insert(responses.begin(), responses.end());
  for (size_t i = 0, end = features.size(); i < end; ++i)
    data.emplace_back(std::move(features[i]), responses[i]);

  root = build_tree(data.begin(),
                    data.end(),
                    /* max_depth */ max_depth,
                    /* depth */ 0);

#ifdef ENABLE_JIT_DTREE
  compile_and_link_jit_evaluate_function();
#endif
}


void DecisionTreeImpl::generate_source(Node &node, OutputFormatter &source_fmt)
{
  if (!node.left && !node.right) {
    source_fmt << "return " & node.predicted_class & ";\n";
    return;
  }

  source_fmt << "if (features[" & node.feature_idx & "] < " & node.threshold &
      ") {\n";
  if (node.left) {
    ++source_fmt;
    generate_source(*node.left, source_fmt);
    --source_fmt;
    source_fmt << "}\n";
  }
  if (node.right) {
    source_fmt << "else {\n";
    ++source_fmt;
    generate_source(*node.right, source_fmt);
    --source_fmt;
    source_fmt << "}\n";
  }
}

void DecisionTreeImpl::compile_and_link_jit_evaluate_function()
{
#ifdef ENABLE_JIT_DTREE
  std::string function_name =
      std::string("_jit_eval_") + std::to_string(unique_id);
  std::string shared_library_name =
      std::string("/tmp/") + function_name + std::string(".so");
  std::stringstream sstream;
  OutputFormatter source_fmt(sstream);
  // source_fmt << "#include <cstdio>\n";
  source_fmt << "#include <vector>\n";
  source_fmt << "extern \"C\" {\n";
  ++source_fmt;
  source_fmt << "int " & function_name &
      "(const std::vector<float> &features) {\n";
  ++source_fmt;
  generate_source(*root, source_fmt);
  --source_fmt;
  source_fmt << "}\n";
  --source_fmt;
  source_fmt << "}\n";
  std::ofstream ofs(std::string("/tmp/" + function_name + std::string(".cpp")));
  ofs << sstream.str() << std::endl;
  ofs.close();
  std::string command = std::string("c++ -x c++ -O0 -shared -fPIC -o ") +
                        shared_library_name + std::string(" - << EOF\n") +
                        sstream.str() + std::string("EOF\n");
  system(command.c_str());
  // pclose(popen(command.c_str(), "r"));
  void *dynamic_linker = dlopen(shared_library_name.c_str(), RTLD_NOW);
  if (!dynamic_linker) {
    std::cerr << "dlopen: " << dlerror() << std::endl;
    abort();
  }
  jit_evaluate_function =
      (int (*)(const std::vector<float> &))dlsym(dynamic_linker,
                                                 function_name.c_str());
  if (!jit_evaluate_function) {
    std::cerr << "dlsym: " << dlerror() << std::endl;
    abort();
  }
#else
  throw std::runtime_error(
      "DTree JIT requires compilation with ENABLE_JIT_DTREE on");
#endif
}

DecisionTreeImpl::~DecisionTreeImpl()
{
  std::string function_name =
      std::string("_jit_eval_") + std::to_string(unique_id);
  std::string shared_library_name =
      std::string("/tmp/") + function_name + std::string(".so");
  std::remove(shared_library_name.c_str());

  for (Node *node : tree_nodes)
    delete node;
}

void DecisionTreeImpl::save(const std::string &filename)
{
  std::ofstream ofs(filename, std::ofstream::out);
  OutputFormatter outfmt(ofs);
  outfmt << "# DecisionTreeImpl\n";
  output_tree(outfmt, "tree");
  ofs.close();
}

int DecisionTreeImpl::predict(const std::vector<float> &features)
{
#ifdef ENABLE_JIT_DTREE
  return jit_evaluate_function(features);
#else
  return evaluate_tree(*root, features);
#endif
}

void DecisionTreeImpl::print_tree()
{
  OutputFormatter outfmt(std::cout);
  output_tree(outfmt, "tree", /*include_data=*/false);
}

DecisionTreeImpl::Node::Node(DecisionTreeImpl &DT,
                             float gini,
                             size_t num_samples,
                             int predicted_class,
                             int feature_idx,
                             float threshold,
                             Node *left,
                             Node *right,
                             std::vector<size_t> &count_per_class)
    : DT(DT),
      gini(gini),
      num_samples(num_samples),
      predicted_class(predicted_class),
      feature_idx(feature_idx),
      threshold(threshold),
      left(left),
      right(right),
      count_per_class(std::move(count_per_class))
{
}

DecisionTreeImpl::Node::Node(DecisionTreeImpl &DT,
                             std::vector<size_t> &count_per_class,
                             float gini,
                             size_t num_samples)
    : DT(DT),
      gini(gini),
      num_samples(num_samples),
      count_per_class(std::move(count_per_class)),
      feature_idx(-1),
      threshold(0),
      left(nullptr),
      right(nullptr)
{
  // Find predicted class as the maximum element in the
  // count_per_class.
  int idx = std::distance(count_per_class.begin(),
                          std::max_element(count_per_class.begin(),
                                           count_per_class.end()));
  predicted_class = *std::next(DT.classes.begin(), idx);
}

void DecisionTreeImpl::load(const std::string &filename)
{
  std::ifstream ifs(filename);
  if (!ifs) throw std::runtime_error("Error loading file: " + filename);
  parse_tree(ifs);
  ifs.close();
}

template <typename Iterator>
void DecisionTreeImpl::compute_gini(const Iterator &Begin,
                                    const Iterator &End,
                                    float &gini_score)
{
  float g = 0.0f;
  size_t n_total = std::distance(Begin, End);

  for (auto it = classes.begin(), end = classes.end(); it != end; ++it) {
    int cls = *it;
    int count = 0;
    for (auto It = Begin; It != End; ++It) {
      int data_cls = It->second;
      if (data_cls == cls) count++;
    }

    float p = (float)count / n_total;
    g += (p * p);
  }

  gini_score = 1.0f - g;
}

template <typename Iterator>
std::vector<size_t> DecisionTreeImpl::get_count_per_class(const Iterator &Begin,
                                                          const Iterator &End)
{
  std::vector<size_t> count_per_class;

  for (auto it = classes.begin(), end = classes.end(); it != end; ++it) {
    int cls = *it;
    int count = 0;
    for (auto It = Begin; It != End; ++It) {
      int data_cls = It->second;
      if (data_cls == cls) count++;
    }

    count_per_class.push_back(count);
  }


  return count_per_class;
}

int DecisionTreeImpl::evaluate_tree(const Node &tree,
                                    const std::vector<float> &features)
{
  // leaf node, return predicted class.
  if (tree.feature_idx == -1) return tree.predicted_class;

  if (features[tree.feature_idx] < tree.threshold) {
    if (tree.left) return evaluate_tree(*tree.left, features);
  } else if (tree.right)
    return evaluate_tree(*tree.right, features);
  return tree.predicted_class;
}

// Returns tuple(min gini, iterator to split, feature index of split)
template <typename Iterator>
std::tuple<float, Iterator, size_t, float> DecisionTreeImpl::split(
    const Iterator &Begin,
    const Iterator &End)
{
  size_t size = std::distance(Begin, End);
  assert(size > 1);

  float min_gini = 1.0f;
  Iterator split_it = End;
  int split_feature_idx = 0;
  float split_threshold = 0.0f;

  size_t num_features = Begin->first.size();
  for (int feature_idx = 0; feature_idx < num_features; ++feature_idx) {
    // Sort based on this specific feature index (sorting does not
    // invalidate iterators).
    std::sort(Begin,
              End,
              [&feature_idx](const std::pair<std::vector<float>, int> &x,
                             const std::pair<std::vector<float>, int> &y) {
                return x.first[feature_idx] < y.first[feature_idx];
              });
    for (Iterator It = std::next(Begin); It != End; ++It) {
      float feature_val_left = std::prev(It)->first[feature_idx];
      float feature_val_right = It->first[feature_idx];
      // Feature values are the same, continue.
      if (feature_val_left == feature_val_right) continue;


      float left_gini_score, right_gini_score;
      compute_gini(Begin, It, left_gini_score);
      compute_gini(It, End, right_gini_score);

      size_t idx = std::distance(Begin, It);
      // weighted gini
      float g =
          (idx * left_gini_score + (size - idx) * right_gini_score) / size;

      // This split does not reduce gini, continue
      if (g >= min_gini) continue;
      min_gini = g;
      split_it = It;
      split_feature_idx = feature_idx;
      split_threshold = (feature_val_left + feature_val_right) / 2.0;
    }
  }

  // Sort to selected feature idx to validate the returned split iterator.
  std::sort(Begin,
            End,
            [&split_feature_idx](const std::pair<std::vector<float>, int> &x,
                                 const std::pair<std::vector<float>, int> &y) {
              return x.first[split_feature_idx] < y.first[split_feature_idx];
            });
  return std::make_tuple(min_gini,
                         split_it,
                         split_feature_idx,
                         split_threshold);
}

template <typename Iterator>
DecisionTreeImpl::Node *DecisionTreeImpl::build_tree(const Iterator &Begin,
                                                     const Iterator &End,
                                                     const size_t max_depth,
                                                     size_t depth)
{
  // std::string s = std::string("build_tree@") + std::to_string(depth);
  // TimeTrace t(s);
  float gini_score;
  compute_gini(Begin, End, gini_score);
  std::vector<size_t> count_per_class = get_count_per_class(Begin, End);
  size_t size = std::distance(Begin, End);
  Node *node = new Node(*this, count_per_class, gini_score, size);
  tree_nodes.push_back(node);

  // Return if max_depth reached.
  if (depth >= max_depth) return node;

  // Return if there is only one sample.
  if (Begin == End) return node;

  // Return if the gini is 0 (samples of the same class).
  if (gini_score <= 0.0f) return node;

  // Split to minimize Gini impurity.
  float min_gini;
  Iterator split_it;
  int split_feature_idx;
  float threshold;
  std::tie(min_gini, split_it, split_feature_idx, threshold) =
      split(Begin, End);
  assert(split_it != End);

  node->feature_idx = split_feature_idx;
  node->threshold = threshold;
  node->left = build_tree(Begin, split_it, max_depth, depth + 1);
  node->right = build_tree(split_it, End, max_depth, depth + 1);

  return node;
}

void DecisionTreeImpl::output_tree(OutputFormatter &outfmt,
                                   std::string key,
                                   bool include_data)
{
  outfmt << key & ": {\n";
  ++outfmt;
  outfmt << "max_depth: " & max_depth & ",\n";
  outfmt << "classes: [ ";
  for (auto it : classes)
    outfmt &it & ", ";
  outfmt & "],\n";
  outfmt << "num_features: " & num_features & ",\n";
  output_node(outfmt, *root, "root");
  if (include_data) {
    outfmt << "data: {\n";
    ++outfmt;
    unsigned idx = 0;
    for (auto it : data) {
      outfmt << idx & ": { ";
      outfmt & "features: [ ";
      for (auto features_it : it.first)
        outfmt &features_it & ", ";
      outfmt & "], ";
      outfmt & "class: " & it.second;
      outfmt & " },\n";
      ++idx;
    }
    --outfmt;
  }
  outfmt << "},\n";
  --outfmt;
  outfmt << "}\n";
}

void DecisionTreeImpl::output_node(OutputFormatter &outfmt,
                                   Node &tree,
                                   std::string key)
{
  outfmt << key & ": {\n";

  ++outfmt;
  outfmt << "gini: " & tree.gini & ",\n";
  outfmt << "num_samples: " & tree.num_samples & ",\n";
  outfmt << "predicted_class: " & tree.predicted_class & ",\n";
  outfmt << "feature_idx: " & tree.feature_idx & ",\n";
  outfmt << "threshold: " & tree.threshold & ",\n";
  outfmt << "count_per_class: {\n";
  for (int i = 0; i < classes.size(); ++i) {
    auto it = std::next(classes.begin(), i);
    int cls = *it;
    outfmt << cls & ": " & tree.count_per_class[i] & ",\n";
  }
  outfmt << "},\n";

  if (tree.left) output_node(outfmt, *tree.left, "left");
  if (tree.right) output_node(outfmt, *tree.right, "right");
  --outfmt;

  outfmt << "},\n";
}

std::vector<size_t> DecisionTreeImpl::parse_count_per_class(Parser &parser)
{
  std::vector<size_t> count_per_class;
  while (!parser.getNextTokenEquals("},")) {
    int key;
    size_t val;
    parser.parse(key);
    parser.parseExpected(":");
    parser.getNextToken();
    parser.parse(val);
    parser.parseExpected(",");
    count_per_class.push_back(val);
  }

  return count_per_class;
}

void DecisionTreeImpl::parse_data(Parser &parser)
{
  parser.getNextToken();
  parser.parseExpected("data:");
  parser.getNextToken();
  parser.parseExpected("{");

  while (!parser.getNextTokenEquals("},")) {
    parser.getNextToken();
    parser.parseExpected("{");

    parser.getNextToken();
    parser.parseExpected("features:");

    parser.getNextToken();
    parser.parseExpected("[");

    std::vector<float> features;
    features.reserve(num_features);
    int response;
    while (!parser.getNextTokenEquals("],")) {
      float feature;
      parser.parse(feature);
      parser.parseExpected(",");
      features.push_back(feature);
    }

    parser.getNextToken();
    parser.parseExpected("class:");

    parser.getNextToken();
    parser.parse(response);

    data.emplace_back(std::move(features), response);

    parser.getNextToken();
    parser.parseExpected("},");
  }
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

DecisionTreeImpl::Node *DecisionTreeImpl::parse_node(Parser &parser)
{
  float gini;
  size_t num_samples;
  int predicted_class;
  int feature_idx;
  float threshold;
  Node *left = nullptr;
  Node *right = nullptr;
  ;

  parser.getNextToken();
  parser.parseExpected("{");

  parseKeyVal(parser, "gini:", gini);
  parseKeyVal(parser, "num_samples:", num_samples);
  parseKeyVal(parser, "predicted_class:", predicted_class);
  parseKeyVal(parser, "feature_idx:", feature_idx);
  parseKeyVal(parser, "threshold:", threshold);

  parser.getNextToken();
  parser.parseExpected("count_per_class:");
  parser.getNextToken();
  parser.parseExpected("{");
  std::vector<size_t> count_per_class = parse_count_per_class(parser);

  parser.getNextToken();
  if (parser.getTokenEquals("left:")) {
    left = parse_node(parser);
    parser.getNextToken();
  }
  if (parser.getTokenEquals("right:")) {
    right = parse_node(parser);
    parser.getNextToken();
  }

  parser.parseExpected("},");

  Node *node = new Node(*this,
                        gini,
                        num_samples,
                        predicted_class,
                        feature_idx,
                        threshold,
                        left,
                        right,
                        count_per_class);

  tree_nodes.push_back(node);
  return node;
}

void DecisionTreeImpl::parse_tree(std::istream &is)
{
  Parser parser(is);

  parser.getNextToken();
  parser.parseExpected("tree:");

  parser.getNextToken();
  parser.parseExpected("{");

  parseKeyVal(parser, "max_depth:", max_depth);

  parser.getNextToken();
  parser.parseExpected("classes:");
  parser.getNextToken();
  parser.parseExpected("[");
  while (!parser.getNextTokenEquals("],")) {
    int class_idx;
    parser.parse<int>(class_idx);
    parser.parseExpected(",");
    classes.insert(class_idx);
  }

  parseKeyVal(parser, "num_features:", num_features);

  parser.getNextToken();
  parser.parseExpected("root:");
  root = parse_node(parser);
  parse_data(parser);

  parser.getNextToken();
  parser.parseExpected("}");
}
