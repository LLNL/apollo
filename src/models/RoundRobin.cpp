// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/RoundRobin.h"

#include <iostream>
#include <string>

#include "apollo/Config.h"

int RoundRobin::getIndex(std::vector<float> &features)
{
  int choice = (last_policy + 1) % policy_count;
  last_policy = choice;
  return choice;

#if 0
    int choice;
    if( policies.find( features ) == policies.end() ) {
        policies[ features ] = 0;
    }
    else {
        // made a full circle, stop
        if( policies[ features ] == 0 )
            return 0;
    }
    choice = policies[ features ];
    policies[ features ] = ( policies[ features ] + 1) % policy_count;
    //std::cout \
        << "features "; \
        for(auto &f : features ) { \
            std::cout << (int) f << ", "; \
        } \
    std::cout << " policy_choice " << choice \
        << " next " << policies[ features ] \
        << std::endl;

    return choice;
#endif
}

RoundRobin::RoundRobin(int num_policies)
    : PolicyModel(num_policies, "RoundRobin")
{
  // TODO: Distributed RoundRobin uses mpi rank for offset.
  last_policy = -1;

  return;
}

RoundRobin::~RoundRobin() { return; }
