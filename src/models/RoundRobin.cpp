
// Copyright (c) 2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <string>
#include <iostream>

#include "apollo/Config.h"
#include "apollo/models/RoundRobin.h"

int
RoundRobin::getIndex(std::vector<float> &features)
{
    int choice = policy_choice;

    policy_choice = ( policy_choice + 1 ) % policy_count;
    policy_choice += offset;

    return choice;
}

RoundRobin::RoundRobin(
        int   num_policies)
    : Model(num_policies, "RoundRobin", true)
{
    int rank = 0;
    char *slurm_procid = getenv("SLURM_PROCID");

    if (slurm_procid != NULL) {
       rank = atoi(slurm_procid);
    } else {
       rank = 0;
    };

#if APOLLO_COLLECTIVE_TRAINING
    int numProcs         = std::stoi(getenv("SLURM_NPROCS"));
    policy_count = num_policies/numProcs;
    int rem = num_policies%numProcs;
    // Give rank 0 any remainder policies
    if( rank == 0 ) {
        policy_count += rem;
        offset = 0; //rank * policy_count;
    }
    else {
        offset = rank * policy_count + rem;
    }

    policy_choice = rank + offset;
#elif APOLLO_LOCAL_TRAINING
    policy_choice = 0;
    offset = 0;
#else
#error "Missing training configuration"
#endif

    //std::cout << "rank " << rank << " policy_count " << policy_count \
        << " offset " << offset << std::endl; //ggout

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
