// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <iostream>
#include <chrono>

#include "apollo/Apollo.h"
#include "apollo/Region.h"

#define NUM_FEATURES 1
#define NUM_POLICIES 4
#define REPS         4
#define DELAY        0.01
#define FREQ 1


static void delay_loop(const double delay)
{
  auto start = std::chrono::steady_clock::now();
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  // Artificial delay when feature mismatches policy.
  while (elapsed.count() < delay) {
    end = std::chrono::steady_clock::now();
    elapsed = end - start;
  }
}

int main()
{
  fprintf(stdout, "testing Apollo.\n");

  int rank;

  Apollo *apollo = Apollo::instance();

  Apollo::Region *r =
      new Apollo::Region(NUM_FEATURES, "test-region1", NUM_POLICIES);

  int match;
  // Outer loop to simulate iterative execution of inner region, install tuned
  // model after first iteration.
  for (int j = 1; j <= 20; j++) {
    match = 0;
    // Features match policies, iterate over all possible pairs when RoundRobin.
    // Do so REPS times to gather multiple measurements per pair.
    for (int n = 0; n < REPS; n++) {
      for (int i = 0; i < NUM_POLICIES; i++) {
        for (int k = 0; k < NUM_POLICIES; k++) {
          int feature = (k + i) % NUM_POLICIES;
          r->begin();
          r->setFeature(float(feature));

          int policy = r->getPolicyIndex();

          printf("Feature %d Policy %d\n", feature, policy);

          if (policy != feature) {
            delay_loop(DELAY);
          } else {
            // No delay when feature matches the policy.
            printf("Match!\n");
            match++;
          }

          r->end();
        }
      }
    }

    // Second outer loop iteration should have perfect matching.
    printf("matched region j %d %d / %d\n", j, match, REPS*NUM_POLICIES*NUM_POLICIES);
    if (j%FREQ == 0) {
      printf("Install model\n");
      apollo->train(0);
    }
  }

  fprintf(stdout, "testing complete.\n");
  if (match == REPS * NUM_POLICIES * NUM_POLICIES)
    printf("PASSED\n");
  else
    printf("FAILED\n");

  return 0;
}
