// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/Region.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "apollo/Apollo.h"
#include "apollo/ModelFactory.h"
#include "helpers/ErrorHandling.h"
#include "timers/TimerSync.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif  // ENABLE_MPI

static inline bool fileExists(std::string path)
{
  struct stat stbuf;
  return (stat(path.c_str(), &stbuf) == 0);
}

void Apollo::Region::train(int step, bool doCollectPendingContexts, bool force)
{
  if (!force)
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
    std::string filename =
        Config::APOLLO_OUTPUT_DIR + "/" + Config::APOLLO_MODELS_DIR + "/" +
        model->name + "-step-" + std::to_string(step) + "-rank-" +
        std::to_string(apollo->mpiRank) + "-" + name + ".yaml";
    model->store(filename);
    filename = Config::APOLLO_OUTPUT_DIR + "/" + Config::APOLLO_MODELS_DIR +
               "/" + model->name +
               "-latest"
               "-rank-" +
               std::to_string(apollo->mpiRank) + "-" + name + ".yaml";
    model->store(filename);

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

// TODO: expand validation to parameter values.
static void validate(const std::string &model_name,
                     std::unordered_map<std::string, std::string> &model_params)
{
  if (model_name == "Static") {
    //  "(policy)=([0-9]+)"
    for (auto &entry : model_params)
      if (entry.first != "policy")
        fatal_error("Unknown param key \"" + entry.first +
                    "\" for policy Static");

    return;
  }

  if (model_name == "DecisionTree") {
    // "(max_depth)=([0-9]+)"
    // "(explore)=(RoundRobin|Random)"
    // "(load)"
    // "(load-dataset)"
    // "(load)=([a-zA-Z0-9_\\-\\.]+)"
    for (auto &entry : model_params)
      if (entry.first != "max_depth" && entry.first != "explore" &&
          entry.first != "load" && entry.first != "load-dataset")
        fatal_error("Unknown param key \"" + entry.first +
                    "\" for policy DecisionTree");

    return;
  }

  if (model_name == "RandomForest") {
    // "(num_trees|max_depth)=([0-9]+)"
    // "(explore)=(RoundRobin|Random)"
    // "(load)"
    // "(load-dataset)"
    // "(load)=([a-zA-Z0-9_\\-\\.]+)"
    for (auto &entry : model_params)
      if (entry.first != "num_trees" && entry.first != "max_depth" &&
          entry.first != "explore" && entry.first != "load" &&
          entry.first != "load-dataset")
        fatal_error("Unknown param key \"" + entry.first +
                    "\" for policy RandomForst");
    return;
  }

  if (model_name == "PolicyNet") {
    for (auto &entry : model_params)
      //  "(lr|beta|beta1|beta2|threshold)=(([+-]?([[:d:]]*\\.?([[:d:]]*)?))([Ee]"
      // "(load)"
      // "(load)=([a-zA-Z0-9_\\-\\.]+)"
      //  "(load-dataset)"
      if (entry.first != "lr" && entry.first != "beta" &&
          entry.first != "beta1" && entry.first != "beta2" &&
          entry.first != "threshold" && entry.first != "load" &&
          entry.first != "load-dataset")
        fatal_error("Unknown param key \"" + entry.first +
                    "\" for policy PolicyNet");
    return;
  }

  fatal_error("Unknow param for policy " + model_name +
              ", policy does not have params");
}

void Apollo::Region::parsePolicyModel(const std::string &model_info)
{
  size_t pos = model_info.find(",");
  model_name = model_info.substr(0, pos);

  // Parse any parameters, return if there are not any.
  if (std::string::npos == pos) return;

  std::string model_params_str = model_info.substr(pos + 1);
  do {
    pos = model_params_str.find(",");
    std::string keyval_str = model_params_str.substr(0, pos);

    size_t kv_pos = keyval_str.find("=");
    if (std::string::npos == kv_pos) {
      model_params.emplace(keyval_str, "");
    } else {
      std::string key = keyval_str.substr(0, kv_pos);
      std::string val = keyval_str.substr(kv_pos + 1);
      model_params.emplace(key, val);
    }

    model_params_str = model_params_str.substr(pos + 1);

  } while (std::string::npos != pos);

  validate(model_name, model_params);
}

Apollo::Region::Region(const int num_features,
                       const char *regionName,
                       int num_policies,
                       int min_training_data,
                       const std::string &_model_info,
                       const std::string &modelYamlFile)
    : num_features(num_features),
      num_policies(num_policies),
      min_training_data(min_training_data),
      model_info(_model_info),
      current_context(nullptr),
      idx(0),
      sync_context()
{
  apollo = Apollo::instance();

  // Create timer for the singleton sync context.
  sync_context.timer = Apollo::Timer::create<Apollo::Timer::Sync>();

  strncpy(name, regionName, sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';

  // If there is no model_info, parse the policy model from the env
  // variable, else parse it from the model_info argument.
  if (model_info.empty()) model_info = Config::APOLLO_POLICY_MODEL;
  parsePolicyModel(model_info);

  model = apollo::ModelFactory::createPolicyModel(model_name,
                                                  num_features,
                                                  num_policies,
                                                  model_params);

  if (!modelYamlFile.empty()) model->load(modelYamlFile);

  if (model_params.count("load")) {
    std::string model_file;
    if (model_params["load"].empty())
      model_file = Config::APOLLO_OUTPUT_DIR + "/" + Config::APOLLO_MODELS_DIR +
                   "/" + model_name + "-latest-rank-" +
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

  if (Config::APOLLO_PERSISTENT_DATASETS) {
    std::string dataset_file = Config::APOLLO_OUTPUT_DIR + "/" +
                               Config::APOLLO_DATASETS_DIR + "/Dataset-" +
                               std::string(name) + ".yaml";
    std::ifstream ifs(dataset_file);
    if (ifs) dataset.load(ifs);
    ifs.close();

    // Check if auto-training applies: min_training_data is set and dataset
    // size is large enough.
    autoTrain();
  }

  if (model_params.count("load-dataset")) {
    std::string dataset_file = Config::APOLLO_OUTPUT_DIR + "/" +
                               Config::APOLLO_DATASETS_DIR + "/Dataset-" +
                               std::string(name) + ".yaml";
    std::ifstream ifs(dataset_file);
    if (!ifs) {
      std::cerr << "ERROR: could not load dataset file " << dataset_file
                << std::endl;
      abort();
    }

    dataset.load(ifs);

    if (dataset.size() <= 0) fatal_error("dataset size is 0");

    // Train with whatever data existing in the dataset, ignore
    // min_training_data (if set).
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

  if (model_name == "DatasetMap") {
    std::string dataset_file = Config::APOLLO_OUTPUT_DIR + "/" +
                               Config::APOLLO_DATASETS_DIR + "/Dataset-" +
                               std::string(name) + ".yaml";
    std::ifstream ifs(dataset_file);
    if (!ifs) {
      std::cerr << "ERROR: could not load dataset file " << dataset_file
                << std::endl;
      abort();
    }

    dataset.load(ifs);

    ifs.close();

    if (dataset.size() <= 0) fatal_error("DatasetMap expects loaded datasets");

    model->train(dataset);
  }

  if (Config::APOLLO_TRACE_CSV) {
    std::string fname(Config::APOLLO_OUTPUT_DIR + "/" +
                      Config::APOLLO_TRACES_DIR + "/trace-" + model_info +
                      "-region-" + name + "-rank-" +
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

  if (Config::APOLLO_PERSISTENT_DATASETS) {
    std::ofstream file_out(Config::APOLLO_OUTPUT_DIR + "/" +
                           Config::APOLLO_DATASETS_DIR + "/Dataset-" +
                           std::string(name) + ".yaml");
    if (!file_out.is_open())
      std::cerr << "ERROR: Cannot write dataset of " + std::string(name) +
                       " to database\n";
    dataset.store(file_out);
    file_out.close();
  }

  return;
}

void Apollo::Region::destroyRegionContext(Apollo::RegionContext *context)
{
  if (context == &sync_context) return;

  delete context;
}

Apollo::RegionContext *Apollo::Region::createRegionContext(TimingKind tk)
{
  Apollo::RegionContext *context = nullptr;
  switch (tk) {
    case TIMING_SYNC:
      context = &sync_context;
      // Clear the features vector because the sync_context is persistent.
      context->features.clear();
      break;
    case TIMING_CUDA_ASYNC:
      context = new Apollo::RegionContext();
      context->timer = Apollo::Timer::create<Apollo::Timer::CudaAsync>();
      break;
    case TIMING_HIP_ASYNC:
      context = new Apollo::RegionContext();
      context->timer = Apollo::Timer::create<Apollo::Timer::HipAsync>();
      break;
    default:
      fatal_error("Cannot resolve timing kind");
  }

  // Pre-allocate the features vector of known size.
  context->features.reserve(num_features);

  return context;
}

Apollo::RegionContext *Apollo::Region::begin() { return begin(TIMING_SYNC); }

Apollo::RegionContext *Apollo::Region::begin(TimingKind tk)
{
  Apollo::RegionContext *context = createRegionContext(tk);

  if (!context) fatal_error("Expected non-null context pointer");

  current_context = context;
  context->idx = this->idx;
  this->idx++;

  context->timer->start();

  return context;
}

Apollo::RegionContext *Apollo::Region::begin(const std::vector<float> &features)
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

  dataset.insert(context->features, context->policy, metric);

  apollo->region_executions++;

  autoTrain();

  destroyRegionContext(context);
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
