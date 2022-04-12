// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/Region.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "apollo/Apollo.h"
#include "apollo/ModelFactory.h"
#include "timers/TimerSync.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif  // ENABLE_MPI

static inline bool fileExists(std::string path)
{
  struct stat stbuf;
  return (stat(path.c_str(), &stbuf) == 0);
}

void Apollo::Region::train(int step, bool doCollectPendingContexts)
{
  if (!model->isTrainable()) return;

  // Conditionally collect pending contexts. Auto-training must not
  // collect contexts to avoid infinite recursion since auto-training
  // happens within collectContext().
  if (doCollectPendingContexts) collectPendingContexts();

  if (dataset.size() <= 0) return;

  if (!Config::APOLLO_REGION_MODEL)
    throw std::runtime_error("Expected per-region model training");

  if (Config::APOLLO_TRACE_BEST_POLICIES) {
    std::stringstream trace_out;
    trace_out << "=== Rank " << apollo->mpiRank << " BEST POLICIES Region "
              << name << " ===" << std::endl;

    std::vector<std::vector<float>> features;
    std::vector<int> responses;
    std::map<std::vector<float>, std::pair<int, double>> min_metric_policies;
    dataset.findMinMetricPolicyByFeatures(features,
                                          responses,
                                          min_metric_policies);
    for (auto &b : min_metric_policies) {
      trace_out << "[ ";
      for (auto &f : b.first)
        trace_out << (int)f << ", ";
      trace_out << "] P:" << b.second.first << " T: " << b.second.second
                << std::endl;
    }
    std::cout << trace_out.str();
    std::ofstream fout("step-" + std::to_string(step) + "-rank-" +
                       std::to_string(apollo->mpiRank) + "-" + name +
                       "-best_policies.txt");
    fout << trace_out.str();
    fout.close();
  }

  model->train(dataset);

  if (Config::APOLLO_RETRAIN_ENABLE)
#ifdef ENABLE_OPENCV
    time_model = ModelFactory::createRegressionTree(dataset);
#else
    throw std::runtime_error("Retraining requires OpenCV");
#endif

  if (Config::APOLLO_STORE_MODELS) {
    model->store(model->name + "-step-" + std::to_string(step) + "-rank-" +
                 std::to_string(apollo->mpiRank) + "-" + name + ".yaml");
    model->store(model->name +
                 "-latest"
                 "-rank-" +
                 std::to_string(apollo->mpiRank) + "-" + name + ".yaml");

    if (Config::APOLLO_RETRAIN_ENABLE) {
#ifdef ENABLE_OPENCV
      time_model->store("regtree-step-" + std::to_string(step) + "-rank-" +
                        std::to_string(apollo->mpiRank) + "-" + name + ".yaml");
      time_model->store(
          "regtree-latest"
          "-rank-" +
          std::to_string(apollo->mpiRank) + "-" + name + ".yaml");
#else
      throw std::runtime_error("Retraining requires OpenCV");
#endif
    }
  }
}

int Apollo::Region::getPolicyIndex(Apollo::RegionContext *context)
{
  int choice = model->getIndex(context->features);

  if (Config::APOLLO_TRACE_POLICY) {
    std::stringstream trace_out;
    int rank;
    rank = apollo->mpiRank;
    trace_out << "Rank " << rank << " region " << name << " model "
              << model->name << " features [ ";
    for (auto &f : context->features)
      trace_out << (int)f << ", ";
    trace_out << "] policy " << choice << std::endl;
    std::cout << trace_out.str();
    std::ofstream fout("rank-" + std::to_string(rank) + "-policies.txt",
                       std::ofstream::app);
    fout << trace_out.str();
    fout.close();
  }

#if 0
    if (choice != context->policy) {
        std::cout << "Change policy " << context->policy\
            << " -> " << choice << " region " << name \
            << " training " << model->training \
            << " features: "; \
            for(auto &f : apollo->features) \
                std::cout << (int)f << ", "; \
            std::cout << std::endl; //ggout
    } else {
        //std::cout << "No policy change for region " << name << ", policy " << current_policy << std::endl; //gout
    }
#endif
  context->policy = choice;
  return choice;
}

void Apollo::Region::parsePolicyModel(const std::string &model_info)
{
  size_t pos = model_info.find(",");
  model_name = model_info.substr(0, pos);

  // Parse any parameters, return if there are not any.
  if (std::string::npos == pos) return;

  std::vector<std::string> params_regex;
  if (model_name == "Static")
    params_regex.push_back("(policy)=([0-9]+)");
  else if (model_name == "DecisionTree") {
    params_regex.push_back("(max_depth)=([0-9]+)");
    params_regex.push_back("(explore)=(RoundRobin|Random)");
    params_regex.push_back("(load)");
    params_regex.push_back("(load-dataset)");
    params_regex.push_back("(load)=([a-zA-Z0-9_\\-\\.]+)");
  } else if (model_name == "RandomForest") {
    params_regex.push_back("(num_trees|max_depth)=([0-9]+)");
    params_regex.push_back("(explore)=(RoundRobin|Random)");
    params_regex.push_back("(load)");
    params_regex.push_back("(load-dataset)");
    params_regex.push_back("(load)=([a-zA-Z0-9_\\-\\.]+)");
  } else if (model_name == "PolicyNet") {
    params_regex.push_back(
        "(lr|beta|beta1|beta2|threshold)=(([+-]?([[:d:]]*\\.?([[:d:]]*)?))([Ee]"
        "[+-]?[[:d:]]+)?)");
    params_regex.push_back("(load)");
    params_regex.push_back("(load)=([a-zA-Z0-9_\\-\\.]+)");
    params_regex.push_back("(load-dataset)");
  }

  std::string model_params_str = model_info.substr(pos + 1);
  do {
    pos = model_params_str.find(",");
    std::string keyval_str = model_params_str.substr(0, pos);
    model_params_str = model_params_str.substr(pos + 1);

    bool matched = false;
    std::smatch m;
    for (auto &regex_str : params_regex) {
      std::regex regex(regex_str);
      if (std::regex_match(keyval_str, m, regex)) {
        auto key = m[1];
        auto value = m[2];
        model_params[key] = value;
        matched = true;
        // std::cout << "Found key " << key << " value " << value << "\n";
      }
    }

    if (!matched) {
      std::cerr << "ERROR: Parameter " << keyval_str
                << " is not valid for model " << model_name << std::endl;
      abort();
    }
  } while (std::string::npos != pos);
}

void Apollo::Region::parseDataset(std::istream &is)
{
  std::smatch m;

  for (std::string line; std::getline(is, line);) {
    // std::cout << "PARSE LINE: " << line << "\n";
    if (std::regex_match(line, std::regex("#.*")))
      continue;
    else if (std::regex_match(line, std::regex("data: \\{")))
      continue;
    else if (std::regex_match(line,
                              m,
                              std::regex("\\s+[0-9]+: \\{ features: \\[ (.*) "
                                         "\\], "
                                         "policy: ([0-9]+), xtime: (.*) "
                                         "\\},"))) {

      std::vector<float> features;
      int policy;
      double xtime;

      std::string feature_list(m[1]);
      std::regex regex("([+|-]?[0-9]+\\.[0-9]*|[+|-]?[0-9]+),");
      std::sregex_token_iterator i =
          std::sregex_token_iterator(feature_list.begin(),
                                     feature_list.end(),
                                     regex,
                                     /* first sub-match*/ 1);
      std::sregex_token_iterator end = std::sregex_token_iterator();
      for (; i != end; ++i)
        features.push_back(std::stof(*i));
      policy = std::stoi(m[2]);
      xtime = std::stof(m[3]);

      // std::cout << "features: [ ";
      // for(auto &f : features)
      //   std::cout << f << ", ";
      // std::cout << "]\n";
      // std::cout << "policy " << policy << "\n";
      // std::cout << "xtime " << xtime << "\n";
      dataset.insert(features, policy, xtime);
    } else if (std::regex_match(line, std::regex("\\s*\\}")))
      break;
    else
      throw std::runtime_error("Error parsing dataset during loading line: " +
                               line);
  }
}

Apollo::Region::Region(const int num_features,
                       const char *regionName,
                       int num_policies,
                       int min_training_data,
                       const std::string &model_info,
                       const std::string &modelYamlFile)
    : num_features(num_features),
      num_policies(num_policies),
      min_training_data(min_training_data),
      model_info(model_info),
      current_context(nullptr),
      idx(0)
{
  apollo = Apollo::instance();

  strncpy(name, regionName, sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';

  // If there is no model_info, parse the policy model from the env
  // variable, else parse it from the model_info argument.
  if (model_info.empty())
    parsePolicyModel(Config::APOLLO_POLICY_MODEL);
  else
    parsePolicyModel(model_info);

  model = ModelFactory::createPolicyModel(model_name,
                                          num_features,
                                          num_policies,
                                          model_params);

  if (!modelYamlFile.empty()) model->load(modelYamlFile);

  if (model_params.count("load")) {
    std::string model_file;
    if (model_params["load"].empty())
      model_file = model_name + "-latest-rank-" +
                   std::to_string(apollo->mpiRank) + "-" + std::string(name) +
                   ".yaml";
    else
      model_file = model_params["load"];

    if (fileExists(model_file)) {
      // std::cout << "Model Load " << model_file << std::endl;
      model->load(model_file);
    } else {
      std::cerr << "ERROR: could not load model file " << model_file
                << ", abort" << std::endl;
      abort();
    }
  }

  if (model_params.count("load-dataset")) {
    std::string dataset_file = "Dataset-" + std::string(name) + ".yaml";
    std::ifstream ifs(dataset_file);
    if (!ifs) {
      std::cerr << "ERROR: could not load dataset file " << dataset_file
                << std::endl;
      abort();
    }
    parseDataset(ifs);
    train(0);
    ifs.close();
  }

  if (model_name == "Optimal") {
    std::string model_file = "opt-" + std::string(name) + "-rank-" +
                             std::to_string(apollo->mpiRank) + ".txt";
    if (!fileExists(model_file)) {
      std::cerr << "Optimal policy file " << model_file << " does not exist"
                << std::endl;
      abort();
    }

    model->load(model_file);
  }

  if (Config::APOLLO_TRACE_CSV) {
    // TODO: assumes model comes from env, fix to use model provided in the
    // constructor
    // TODO: convert to filesystem C++17 API when Apollo moves to it
    int ret;
    ret = mkdir(
        std::string("./trace" + Config::APOLLO_TRACE_CSV_FOLDER_SUFFIX).c_str(),
        S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (ret != 0 && errno != EEXIST) {
      perror("TRACE_CSV mkdir");
      abort();
    }

    std::string fname("./trace" + Config::APOLLO_TRACE_CSV_FOLDER_SUFFIX +
                      "/trace-" + model_info + "-region-" + name + "-rank-" +
                      std::to_string(apollo->mpiRank) + ".csv");
    std::cout << "TRACE_CSV fname " << fname << std::endl;
    trace_file.open(fname);
    if (trace_file.fail()) {
      std::cerr << "Error opening trace file " + fname << std::endl;
      abort();
    }
    // Write header.
    trace_file << "rankid training region idx";
    // trace_file << "features";
    for (int i = 0; i < num_features; i++)
      trace_file << " f" << i;
    trace_file << " policy xtime\n";
  }
  // std::cout << "Insert region " << name << " ptr " << this << std::endl;
  const auto ret = apollo->regions.insert({name, this});

  return;
}

Apollo::Region::~Region()
{
  // Disable period based flushing.
  Config::APOLLO_GLOBAL_TRAIN_PERIOD = 0;
  while (pending_contexts.size() > 0)
    collectPendingContexts();

  if (Config::APOLLO_TRACE_CSV) trace_file.close();

  return;
}

Apollo::RegionContext *Apollo::Region::begin()
{
  Apollo::RegionContext *context = new Apollo::RegionContext(num_features);
  current_context = context;
  context->idx = this->idx;
  this->idx++;
  // Use the sync timer by default.
  context->timer = Timer::create<Timer::Sync>();
  context->timer->start();

  return context;
}

Apollo::RegionContext *Apollo::Region::begin(std::vector<float> features)
{
  Apollo::RegionContext *context = begin();
  context->features = features;
  return context;
}

void Apollo::Region::autoTrain()
{
  if (!model->isTrainable()) return;

  if (Config::APOLLO_GLOBAL_TRAIN_PERIOD &&
      (apollo->region_executions % Config::APOLLO_GLOBAL_TRAIN_PERIOD) == 0) {
    apollo->train(apollo->region_executions,
                  /* doCollectPendingContexts */ false);
  } else if (Config::APOLLO_PER_REGION_TRAIN_PERIOD &&
             (idx % Config::APOLLO_PER_REGION_TRAIN_PERIOD) == 0) {
    train(idx, /* doCollectPendingContexts */ false);
  } else if (0 < min_training_data && min_training_data <= dataset.size())
    train(idx, /* doCollectPendingContexts */ false);
}

void Apollo::Region::collectContext(Apollo::RegionContext *context,
                                    double metric)
{
  dataset.insert(context->features, context->policy, metric);

  if (Config::APOLLO_TRACE_CSV) {
    trace_file << apollo->mpiRank << " ";
    trace_file << model->name << " ";
    trace_file << this->name << " ";
    trace_file << context->idx << " ";
    for (auto &f : context->features)
      trace_file << f << " ";
    trace_file << context->policy << " ";
    trace_file << metric << "\n";
  }

  apollo->region_executions++;

  autoTrain();

  delete context;
  current_context = nullptr;
}

void Apollo::Region::end(Apollo::RegionContext *context, double metric)
{
  // std::cout << "END REGION " << name << " metric " << metric << std::endl;

  collectContext(context, metric);

  collectPendingContexts();

  return;
}

void Apollo::Region::collectPendingContexts()
{
  auto isDone = [this](Apollo::RegionContext *context) {
    if (!context->timer)
      throw std::runtime_error("No timer has been set for the context");
    double metric;
    if (context->timer->isDone(metric)) {
      collectContext(context, metric);
      return true;
    }

    return false;
  };

  pending_contexts.erase(std::remove_if(pending_contexts.begin(),
                                        pending_contexts.end(),
                                        isDone),
                         pending_contexts.end());
}

void Apollo::Region::end(Apollo::RegionContext *context)
{
  context->timer->stop();
  pending_contexts.push_back(context);
  collectPendingContexts();
}

// DEPRECATED
int Apollo::Region::getPolicyIndex(void)
{
  return getPolicyIndex(current_context);
}

// DEPRECATED
void Apollo::Region::end(double metric) { end(current_context, metric); }

// DEPRECATED
void Apollo::Region::end(void) { end(current_context); }

void Apollo::Region::setFeature(Apollo::RegionContext *context, float value)
{
  context->features.push_back(value);
  return;
}

// DEPRECATED
void Apollo::Region::setFeature(float value)
{
  setFeature(current_context, value);
}
