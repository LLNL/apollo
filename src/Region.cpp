// Copyright (c) 2019-2021, Lawrence Livermore National Security, LLC
// and other Apollo project developers.
// Produced at the Lawrence Livermore National Laboratory.
// See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <utility>

#include "apollo/Apollo.h"
#include "apollo/ModelFactory.h"
#include "apollo/Region.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif //ENABLE_MPI

static inline bool fileExists(std::string path) {
    struct stat stbuf;
    return (stat(path.c_str(), &stbuf) == 0);
}

void Apollo::Region::train(int step)
{
  if (!model->training) return;

  collectPendingContexts();
  reduceBestPolicies(step);
  if (best_policies.size() <= 0) return;

  measures.clear();

  std::vector<std::vector<float> > train_features;
  std::vector<int> train_responses;

  std::vector<std::vector<float> > train_time_features;
  std::vector<float> train_time_responses;

  if (Config::APOLLO_REGION_MODEL) {
    std::cout << "TRAIN MODEL PER REGION " << name << std::endl;
    // Prepare training data
    for (auto &it2 : best_policies) {
      train_features.push_back(it2.first);
      train_responses.push_back(it2.second.first);

      std::vector<float> feature_vector = it2.first;
      feature_vector.push_back(it2.second.first);
      if (Config::APOLLO_RETRAIN_ENABLE) {
#ifdef ENABLE_OPENCV
        train_time_features.push_back(feature_vector);
        train_time_responses.push_back(it2.second.second);
#else
        throw std::runtime_error("Retraining requires OpenCV");
#endif
      }
    }
  } else {
      assert(false && "Expected per-region model.");
  }

  if (Config::APOLLO_TRACE_BEST_POLICIES) {
    std::stringstream trace_out;
    trace_out << "=== Rank " << apollo->mpiRank << " BEST POLICIES Region "
              << name << " ===" << std::endl;
    for (auto &b : best_policies) {
      trace_out << "[ ";
      for (auto &f : b.first)
        trace_out << (int)f << ", ";
      trace_out << "] P:" << b.second.first << " T: " << b.second.second
                << std::endl;
    }
    trace_out << ".-" << std::endl;
    std::cout << trace_out.str();
    std::ofstream fout("step-" + std::to_string(step) + "-rank-" +
                       std::to_string(apollo->mpiRank) + "-" + name +
                       "-best_policies.txt");
    fout << trace_out.str();
    fout.close();
  }

  model = ModelFactory::createTuningModel(model_name,
                                           num_policies,
                                           train_features,
                                           train_responses,
                                           model_params);

  if (Config::APOLLO_RETRAIN_ENABLE)
#ifdef ENABLE_OPENCV
    time_model = ModelFactory::createRegressionTree(train_time_features,
                                                    train_time_responses);
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

int
Apollo::Region::getPolicyIndex(Apollo::RegionContext *context)
{
    int choice = model->getIndex( context->features );

    if( Config::APOLLO_TRACE_POLICY ) {
        std::stringstream trace_out;
        int rank;
        rank = apollo->mpiRank;
        trace_out << "Rank " << rank \
            << " region " << name \
            << " model " << model->name \
            << " features [ ";
        for(auto &f: context->features)
            trace_out << (int)f << ", ";
        trace_out << "] policy " << choice << std::endl;
        std::cout << trace_out.str();
        std::ofstream fout( "rank-" + std::to_string(rank) + "-policies.txt", std::ofstream::app );
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
    //log("getPolicyIndex took ", evaluation_time_total, " seconds.\n");
    return choice;
}

void Apollo::Region::parseTuningModel(std::string &model_info)
{
  size_t pos = model_info.find(",");
  model_name = model_info.substr(0, pos);

  // Parse any parameters, return if there are not any.
  if (std::string::npos == pos) return;

  std::string model_params_str = model_info.substr(pos + 1);
  // TODO: better error checking and reporting for invalid keys. For now they
  // are discarded/ignored.
  std::regex regex("(num_trees|max_depth)=([0-9]+)");
  std::sregex_token_iterator key =
      std::sregex_token_iterator(model_params_str.begin(),
                                 model_params_str.end(),
                                 regex,
                                 /* first sub-match */ 1);
  std::sregex_token_iterator value =
      std::sregex_token_iterator(model_params_str.begin(),
                                 model_params_str.end(),
                                 regex,
                                 /* second sub-match */ 2);
  std::sregex_token_iterator end = std::sregex_token_iterator();

  for (; key != end; ++key, ++value)
    model_params[*key] = *value;
}

Apollo::Region::Region(const int num_features,
                       const char *regionName,
                       int num_policies,
                       Apollo::CallbackDataPool *callbackPool,
                       const std::string &modelYamlFile)
    : num_features(num_features),
      num_policies(num_policies),
      current_context(nullptr),
      idx(0),
      callback_pool(callbackPool)
{
    apollo = Apollo::instance();

    strncpy(name, regionName, sizeof(name)-1 );
    name[ sizeof(name)-1 ] = '\0';

    parseTuningModel(Config::APOLLO_TUNING_MODEL);

    if (!modelYamlFile.empty())
      model = ModelFactory::createTuningModel(model_name,
                                               num_policies,
                                               modelYamlFile);
    else {
        // TODO use best_policies to train a model for new region for which there's training data
        size_t pos = Config::APOLLO_INIT_MODEL.find(",");
        std::string model_str = Config::APOLLO_INIT_MODEL.substr(0, pos);
        if ("Static" == model_str)
        {
            int policy_choice = std::stoi(Config::APOLLO_INIT_MODEL.substr(pos + 1));
            if (policy_choice < 0 || policy_choice >= num_policies)
            {
                std::cerr << "Invalid policy_choice " << policy_choice << std::endl;
                abort();
            }
            model = ModelFactory::createStatic(num_policies, policy_choice);
            //std::cout << "Model Static policy " << policy_choice << std::endl;
        }
        else if ("Load" == model_str)
        {
            std::string model_file;
            if (pos == std::string::npos)
            {
                // Load per region model using the region name for the model file.
                model_file = model_name + "-latest-rank-" +
                             std::to_string(apollo->mpiRank) + "-" +
                             std::string(name) + ".yaml";
            }
            else
            {
                // Load the same model for all regions.
                model_file = Config::APOLLO_INIT_MODEL.substr(pos + 1);
            }

            if (fileExists(model_file))
                //std::cout << "Model Load " << model_file << std::endl;
                model = ModelFactory::createTuningModel(model_name,
                                                         num_policies,
                                                         model_file);
            else {
                // Fallback to default model.
                std::cout << "WARNING: could not load file " << model_file
                          << ", falling back to default Static, 0" << std::endl;
                model = ModelFactory::createStatic(num_policies, 0);
            }
        }
        else if ("Random" == model_str)
        {
            model = ModelFactory::createRandom(num_policies);
            //std::cout << "Model Random" << std::endl;
        }
        else if ("RoundRobin" == model_str)
        {
            model = ModelFactory::createRoundRobin(num_policies);
            //std::cout << "Model RoundRobin" << std::endl;
        }
        else if ("Optimal" == model_str) {
           std::string file = "opt-" + std::string(name) + "-rank-" + std::to_string(apollo->mpiRank) + ".txt";
           if (!fileExists(file)) {
               std::cerr << "Optimal policy file " << file << " does not exist" << std::endl;
               abort();
           }

           model = ModelFactory::createOptimal(file);
        }
        else
        {
            std::cerr << "Invalid model env var: " + Config::APOLLO_INIT_MODEL << std::endl;
            abort();
        }
    }

    if( Config::APOLLO_TRACE_CSV ) {
        // TODO: assumes model comes from env, fix to use model provided in the constructor
        // TODO: convert to filesystem C++17 API when Apollo moves to it
        int ret;
        ret = mkdir(
            std::string("./trace" + Config::APOLLO_TRACE_CSV_FOLDER_SUFFIX)
                .c_str(),
            S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (ret != 0 && errno != EEXIST) {
            perror("TRACE_CSV mkdir");
            abort();
        }
        std::string fname("./trace" + Config::APOLLO_TRACE_CSV_FOLDER_SUFFIX +
                          "/trace-" + Config::APOLLO_INIT_MODEL + "-region-" +
                          name + "-rank-" + std::to_string(apollo->mpiRank) +
                          ".csv");
        std::cout << "TRACE_CSV fname " << fname << std::endl;
        trace_file.open(fname);
        if(trace_file.fail()) {
            std::cerr << "Error opening trace file " + fname << std::endl;
            abort();
        }
        // Write header.
        trace_file << "rankid training region idx";
        //trace_file << "features";
        for(int i=0; i<num_features; i++)
            trace_file << " f" << i;
        trace_file << " policy xtime\n";
    }
    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    const auto ret = apollo->regions.insert( { name, this } );

    return;
}

Apollo::Region::~Region()
{
    // Disable period based flushing.
    Config::APOLLO_GLOBAL_TRAIN_PERIOD = 0;
    while(pending_contexts.size() > 0)
       collectPendingContexts();

    if(callback_pool)
        delete callback_pool;

    if( Config::APOLLO_TRACE_CSV )
        trace_file.close();

    return;
}

Apollo::RegionContext *
Apollo::Region::begin()
{
    Apollo::RegionContext *context = new Apollo::RegionContext();
    current_context = context;
    context->idx = this->idx;
    this->idx++;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    context->exec_time_begin = ts.tv_sec + ts.tv_nsec/1e9;
    context->isDoneCallback = nullptr;
    context->callback_arg = nullptr;
    return context;
}

Apollo::RegionContext *
Apollo::Region::begin(std::vector<float> features)
{
    Apollo::RegionContext *context = begin();
    context->features = features;
    return context;
}

void
Apollo::Region::collectContext(Apollo::RegionContext *context, double metric)
{
  // std::cout << "COLLECT CONTEXT " << context->idx << " REGION " << name \
            << " metric " << metric << std::endl;
  auto iter = measures.find({context->features, context->policy});
  if (iter == measures.end()) {
    iter = measures
               .insert(std::make_pair(
                   std::make_pair(context->features, context->policy),
                   std::move(
                       std::make_unique<Apollo::Region::Measure>(1, metric))))
               .first;
    } else {
        iter->second->exec_count++;
        iter->second->time_total += metric;
    }

    if( Config::APOLLO_TRACE_CSV ) {
        trace_file << apollo->mpiRank << " ";
        trace_file << model->name << " ";
        trace_file << this->name << " ";
        trace_file << context->idx << " ";
        for(auto &f : context->features)
            trace_file << f << " ";
        trace_file << context->policy << " ";
        trace_file << metric << "\n";
    }

    apollo->region_executions++;

    if( Config::APOLLO_GLOBAL_TRAIN_PERIOD && ( apollo->region_executions%Config::APOLLO_GLOBAL_TRAIN_PERIOD) == 0 ) {
        //std::cout << "FLUSH PERIOD! region_executions " << apollo->region_executions<< std::endl; //ggout
        apollo->flushAllRegionMeasurements(apollo->region_executions);
    }
    else if( Config::APOLLO_PER_REGION_TRAIN_PERIOD && (idx%Config::APOLLO_PER_REGION_TRAIN_PERIOD) == 0 ) {
        train(idx);
    }

    delete context;
    current_context = nullptr;
}

void
Apollo::Region::end(Apollo::RegionContext *context, double metric)
{
    //std::cout << "END REGION " << name << " metric " << metric << std::endl;

    collectContext(context, metric);

    collectPendingContexts();

    return;
}

void Apollo::Region::collectPendingContexts() {
  auto isDone = [this](Apollo::RegionContext *context) {
    bool returnsMetric;
    double metric;
    if (context->isDoneCallback(context->callback_arg, &returnsMetric, &metric)) {
      if (returnsMetric)
        collectContext(context, metric);
      else {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        context->exec_time_end = ts.tv_sec + ts.tv_nsec/1e9;
        double duration = context->exec_time_end - context->exec_time_begin;
        collectContext(context, duration);
      }
      return true;
    }

    return false;
  };

  pending_contexts.erase(
      std::remove_if(pending_contexts.begin(), pending_contexts.end(), isDone),
      pending_contexts.end());
}

void
Apollo::Region::end(Apollo::RegionContext *context)
{
    if(context->isDoneCallback)
        pending_contexts.push_back(context);
    else {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      context->exec_time_end = ts.tv_sec + ts.tv_nsec/1e9;
      double duration = context->exec_time_end - context->exec_time_begin;
      collectContext(context, duration);
    }

    collectPendingContexts();
}


// DEPRECATED
int
Apollo::Region::getPolicyIndex(void)
{
    return getPolicyIndex(current_context);
}

// DEPRECATED
void
Apollo::Region::end(double metric)
{
    end(current_context, metric);
}

// DEPRECATED
void
Apollo::Region::end(void)
{
    end(current_context);
}

int
Apollo::Region::reduceBestPolicies(int step)
{
    std::stringstream trace_out;
    int rank;
    if( Config::APOLLO_TRACE_MEASURES ) {
#ifdef ENABLE_MPI
        rank = apollo->mpiRank;
#else
        rank = 0;
#endif //ENABLE_MPI
        trace_out << "=================================" << std::endl \
            << "Rank " << rank << " Region " << name << " MEASURES "  << std::endl;
    }
    for (auto iter_measure = measures.begin();
            iter_measure != measures.end();   iter_measure++) {

        const std::vector<float>& feature_vector = iter_measure->first.first;
        const int policy_index                   = iter_measure->first.second;
        auto                           &time_set = iter_measure->second;

        if( Config::APOLLO_TRACE_MEASURES ) {
            trace_out << "features: [ ";
            for(auto &f : feature_vector ) { \
                trace_out << (int)f << ", ";
            }
            trace_out << " ]: "
                << "policy: " << policy_index
                << " , count: " << time_set->exec_count
                << " , total: " << time_set->time_total
                << " , time_avg: " <<  ( time_set->time_total / time_set->exec_count ) << std::endl;
        }
        double time_avg = ( time_set->time_total / time_set->exec_count );

        auto iter =  best_policies.find( feature_vector );
        if( iter ==  best_policies.end() ) {
            best_policies.insert( { feature_vector, { policy_index, time_avg } } );
        }
        else {
            // Key exists
            if(  best_policies[ feature_vector ].second > time_avg ) {
                best_policies[ feature_vector ] = { policy_index, time_avg };
            }
        }
    }

    if( Config::APOLLO_TRACE_MEASURES ) {
        trace_out << ".-" << std::endl;
        trace_out << "Rank " << rank << " Region " << name << " Reduce " << std::endl;
        for( auto &b : best_policies ) {
            trace_out << "features: [ ";
            for(auto &f : b.first )
                trace_out << (int)f << ", ";
            trace_out << "]: P:"
                << b.second.first << " T: " << b.second.second << std::endl;
        }
        trace_out << ".-" << std::endl;
        std::cout << trace_out.str();
        std::ofstream fout("step-" + std::to_string(step) +
                "-rank-" + std::to_string(rank) + "-" + name + "-measures.txt"); \
            fout << trace_out.str();
        fout.close();
    }

    return best_policies.size();
}

void
Apollo::Region::setFeature(Apollo::RegionContext *context, float value)
{
    context->features.push_back(value);
    return;
}

// DEPRECATED
void
Apollo::Region::setFeature(float value)
{
    setFeature(current_context, value);
}
