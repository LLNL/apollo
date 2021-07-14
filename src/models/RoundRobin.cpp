
// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <string>
#include <iostream>

#include "apollo/Config.h"
#include "apollo/models/RoundRobin.h"

int
RoundRobin::getIndex(std::vector<float> &features)
{
    int choice = (last_policy + 1)%policy_count;
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

RoundRobin::RoundRobin(
        int   num_policies)
    : PolicyModel(num_policies, "RoundRobin", true)
{
    int rank = 0;
    char *slurm_procid = getenv("SLURM_PROCID");

    if (slurm_procid != NULL) {
       rank = atoi(slurm_procid);
    } else {
       rank = 0;
    };

    last_policy = -1;

    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//

RoundRobin::~RoundRobin()
{
    return;
}
