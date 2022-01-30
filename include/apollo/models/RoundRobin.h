// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <map>
#include <string>
#include <vector>

#include "apollo/PolicyModel.h"

class RoundRobin : public PolicyModel
{
public:
  RoundRobin(int num_policies);
  ~RoundRobin();

  int getIndex(std::vector<float> &features);
  void load(const std::string &filename){};
  void store(const std::string &filename){};
  bool isTrainable() { return false; }
  void train(
      std::vector<std::tuple<std::vector<float>, int, double> > &measures)
  {
  }

private:
  std::map<std::vector<float>, int> policies;
  int last_policy;

};  // end: RoundRobin (class)


#endif
