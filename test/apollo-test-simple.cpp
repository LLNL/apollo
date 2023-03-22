// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <iostream>

#include "apollo/Apollo.h"
#include "apollo/Region.h"

#define NUM_FEATURES 1
#define NUM_POLICIES 2

int main()
{
  std::cout << "=== Apollo: Simple Compilation Testing\n";

  Apollo *apollo = Apollo::instance();

  // Create region.
  Apollo::Region *r =
      new Apollo::Region(NUM_FEATURES, "test-region", NUM_POLICIES);


  for (int i = 0; i < 10; ++i) {
    r->begin();
    r->setFeature(float(1.0));

    int policy = r->getPolicyIndex();

    std::cout << "Executing with policy " << policy << "\n";

    r->end();
  }

  std::cout << "PASSED\n";

  std::cout << "=== Testing complete\n";

  return 0;
}
