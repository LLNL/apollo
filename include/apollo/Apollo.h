// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_H
#define APOLLO_H

#include <map>
#include <string>
#include <vector>

#include "apollo/Config.h"

class Apollo
{
public:
  ~Apollo();
  // disallow copy constructor
  Apollo(const Apollo &) = delete;
  Apollo &operator=(const Apollo &) = delete;

  static Apollo *instance(void) noexcept
  {
    static Apollo the_instance;
    return &the_instance;
  }

  class Region;
  struct RegionContext;
  class Dataset;
  class Timer;

  //
  int mpiSize;  // 1 if no MPI
  int mpiRank;  // 0 if no MPI

  // NOTE(chad): We default to walk_distance of 2 so we can
  //             step out of this method, then step out of
  //             some portable policy template, and get to the
  //             module name and offset where that template
  //             has been instantiated in the application code.
  std::string getCallpathOffset(int walk_distance = 2);
  void *callpath_ptr;

  // DEPRECATED, use train.
  void flushAllRegionMeasurements(int step);
  void train(int step, bool doCollectPendingContext = true);

private:
  Apollo();
  //
  void gatherCollectiveTrainingData(int step);
  // Key: region name, value: region raw pointer
  std::map<std::string, Apollo::Region *> regions;
  // Count total number of region invocations
  unsigned long long region_executions;
};  // end: Apollo

extern "C" {
void *__apollo_region_create(int num_features,
                             const char *id,
                             int num_policies,
                             int min_training_data,
                             const char *model_info);
void __apollo_region_begin(Apollo::Region *r);
void __apollo_region_end(Apollo::Region *r);
void __apollo_region_set_feature(Apollo::Region *r, float feature);
int __apollo_region_get_policy(Apollo::Region *r);
void __apollo_region_train(Apollo::Region *r, int step);
}

#endif
