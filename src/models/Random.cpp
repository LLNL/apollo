// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/Random.h"

#include <iostream>

#include "apollo/Config.h"

namespace apollo
{

int Random::getIndex(std::vector<float> &features)
{
  int choice = 0;

  if (policy_count > 1) {
    choice = random_dist(random_gen);
    //std::cout << "Choose [ " << 0 << ", " << (policy_count - 1) \
            << " ]: " << choice << std::endl;
  } else {
    choice = 0;
  }

  return choice;
}

Random::Random(int num_policies) : PolicyModel(num_policies, "Random")
{
  random_gen = std::mt19937(random_dev());
  random_dist = std::uniform_int_distribution<>(0, policy_count - 1);
}

Random::~Random() { return; }

}  // end namespace apollo.
