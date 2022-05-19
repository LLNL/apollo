// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <sys/resource.h>

#include <chrono>
#include <iostream>

#include "apollo/Apollo.h"
#include "apollo/Region.h"

int main(int argc, char *argv[])
{
  if (argc < 5) {
    std::cerr << "Usage: ./apollo-overhead <num features> <num policies> <num "
                 "training "
                 "data> <reps>\n";
    return 1;
  }

  unsigned NUM_FEATURES = atoi(argv[1]);
  unsigned NUM_POLICIES = atoi(argv[2]);
  unsigned NUM_TRAINING_DATA = atoi(argv[3]);
  unsigned REPS = atoi(argv[4]);

  if (NUM_TRAINING_DATA > REPS) {
    std::cerr << "NUM_TRAINING_DATA " << NUM_TRAINING_DATA
              << " must be less than or equal to than REPS " << REPS << "\n";
    return 1;
  }

  std::cout << "=== Testing Apollo overhead\n";

  Apollo *apollo = Apollo::instance();

  Apollo::Region *r = new Apollo::Region(NUM_FEATURES,
                                         "test-overhead",
                                         NUM_POLICIES,
                                         /* min_training_data */ 0,
                                         "DecisionTree,max_depth=2");

  auto start = std::chrono::steady_clock::now();

  unsigned i;
  int policy;
  for (i = 0; i < REPS; ++i) {
    r->begin();
    for (unsigned j = 0; j < NUM_FEATURES; ++j)
      r->setFeature(i % NUM_TRAINING_DATA);
    // r->setFeature(1);
    // r->setFeature(2);
    // r->setFeature(3);
    // r->setFeature(2);
    policy = r->getPolicyIndex();
    r->end();
  }

  auto end = std::chrono::steady_clock::now();
  double duration = std::chrono::duration<double>(end - start).count();

  start = std::chrono::steady_clock::now();
  for (i = 0; i < REPS; ++i)
    r->train(0, true, true);
  end = std::chrono::steady_clock::now();
  double model_building_duration =
      std::chrono::duration<double>(end - start).count();

  r->begin();
  start = std::chrono::steady_clock::now();
  for (i = 0; i < REPS; ++i)
    r->getPolicyIndex();
  end = std::chrono::steady_clock::now();
  r->end();
  double model_evaluation_duration =
      std::chrono::duration<double>(end - start).count();

  struct rusage rs;
  getrusage(RUSAGE_SELF, &rs);

  std::cout << "Execution time overhead total " << duration * 1e3 << " ms,"
            << " per iteration " << (duration / REPS) * 1e6 << " us\n"
            << "Memory usage (maxrss) " << rs.ru_maxrss / (1024.0 * 1024.0)
            << " MB\n"
            << "Model building overhead total " << model_building_duration * 1e3
            << " ms, "
            << "per iteration " << (model_building_duration / REPS) * 1e6
            << " us\n"
            << "Model evaluation overhead total "
            << model_evaluation_duration * 1e3 << " ms, "
            << "per iteration " << (model_evaluation_duration / REPS) * 1e6
            << " us\n";

  std::cout << "=== Testing complete\n";

  return 0;
}
