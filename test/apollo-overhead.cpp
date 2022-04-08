// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <sys/resource.h>

#include <chrono>
#include <iostream>

#include "apollo/Apollo.h"
#include "apollo/Region.h"

#define NUM_FEATURES 1
#define NUM_POLICIES 4
#define REPS 1000000

int main()
{
  std::cout << "=== Testing Apollo overhead\n";

  Apollo *apollo = Apollo::instance();

  Apollo::Region *r = new Apollo::Region(NUM_FEATURES,
                                         "test-overhead",
                                         NUM_POLICIES,
                                         /* min_training_data */ 0,
                                         "Static,policy=0");
                                         //"RandomForest,num_trees=3");

  auto start = std::chrono::steady_clock::now();

  unsigned i;
  int policy;
  for (i = 0; i < REPS; ++i) {
    r->begin();
    r->setFeature(0);
    // r->setFeature(1);
    // r->setFeature(2);
    // r->setFeature(3);
    // r->setFeature(2);
    policy = r->getPolicyIndex();
    r->end();
  }

  auto end = std::chrono::steady_clock::now();

  double duration = std::chrono::duration<double>(end - start).count();
  struct rusage rs;
  getrusage(RUSAGE_SELF, &rs);

  std::cout << "Execution time overhead total " << duration * 1e3 << " ms,"
            << " per iteration " << (duration / i) * 1e6 << " us \n";
  std::cout << "Memory usage (maxrss) " << rs.ru_maxrss/(1024.0 * 1024.0) << " MB\n";

  std::cout << "=== Testing complete\n";

  return 0;
}
