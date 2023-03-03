// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/Optimal.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace apollo
{

Optimal::Optimal(std::string file) : PolicyModel(0, "Optimal")
{
  std::ifstream infile(file);
  std::string policy_str;

  while (std::getline(infile, policy_str, ','))
    optimal_policy.push_back(std::stoi(policy_str));

  infile.close();
}

Optimal::Optimal() : PolicyModel(0, "Optimal") {}

int Optimal::getIndex(std::vector<float> &features)
{
  if (optimal_policy.empty()) {
    std::cerr << "Optimal policy queue is empty!" << std::endl;
    abort();
  }

  int policy = optimal_policy.front();
  optimal_policy.pop_front();
  return policy;
}

void Optimal::load(const std::string &filename)
{
  std::ifstream infile(filename);
  std::string policy_str;

  while (std::getline(infile, policy_str, ','))
    optimal_policy.push_back(std::stoi(policy_str));

  infile.close();
}

}  // end namespace apollo.