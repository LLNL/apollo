// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <chrono>
#include <fstream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "apollo/Apollo.h"
#include "apollo/Dataset.h"
#include "apollo/PolicyModel.h"
#include "apollo/Timer.h"
#include "apollo/TimingModel.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif  // ENABLE_MPI

struct Apollo::RegionContext {
  std::vector<float> features;
  int policy;
  unsigned long long idx;
  std::unique_ptr<Timer> timer;
};  // end: Apollo::RegionContext

class Apollo::Region
{

public:
  enum TimingKind { TIMING_SYNC, TIMING_CUDA_ASYNC, TIMING_HIP_ASYNC };
  Region(const int num_features,
         const char *regionName,
         int numAvailablePolicies,
         int min_training_data = 0,
         const std::string &model_info = "",
         const std::string &modelYamlFile = "");
  ~Region();

  char name[64];

  // DEPRECATED interface assuming synchronous execution, will be removed
  void end();
  void end(double metric);
  int getPolicyIndex(void);
  void setFeature(float value);
  // END of DEPRECATED

  Apollo::RegionContext *begin();
  Apollo::RegionContext *begin(TimingKind tk);
  Apollo::RegionContext *begin(const std::vector<float> &features);
  void end(Apollo::RegionContext *context);
  void end(Apollo::RegionContext *context, double metric);
  int getPolicyIndex(Apollo::RegionContext *context);
  void setFeature(Apollo::RegionContext *, float value);

  int idx;
  int num_features;
  int num_policies;

  // Stores measurements of features, policy, metric.
  Apollo::Dataset dataset;

  std::unique_ptr<TimingModel> time_model;
  std::unique_ptr<apollo::PolicyModel> model;

  void collectPendingContexts();
  void train(int step,
             bool doCollectPendingContexts = true,
             bool force = false);

  void parsePolicyModel(const std::string &model_info);
  // Model information, name and params.
  std::string model_info;
  std::string model_name;
  std::unordered_map<std::string, std::string> model_params;

private:
  Apollo *apollo;
  // DEPRECATED wil be removed
  Apollo::RegionContext *current_context;
  Apollo::RegionContext sync_context;
  std::ofstream trace_file;

  std::vector<Apollo::RegionContext *> pending_contexts;
  Apollo::RegionContext *createRegionContext(TimingKind tk);
  void destroyRegionContext(Apollo::RegionContext *context);
  void collectContext(Apollo::RegionContext *, double);
  Apollo::RegionContext *getSyncContext();

  void autoTrain();

  int min_training_data;
};  // end: Apollo::Region

#endif
