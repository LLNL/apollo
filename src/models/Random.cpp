
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

#include <random>
#include <iostream> //ggout

#include "apollo/Config.h"
#include "apollo/models/Random.h"

int
Random::getIndex(std::vector<float> &features)
{
    int choice = 0;

    if (policy_count > 1) {
        std::random_device              random_dev;
        std::mt19937                    random_gen(random_dev());
        //std::uniform_int_distribution<> random_dist(0, (policy_count - 1));
        std::uniform_int_distribution<> random_dist(offset, offset + (policy_count - 1));
        // Return a random index, 0..(policy_count - 1):
        choice = random_dist(random_gen);
        //std::cout << "Choose [ " << offset << ", " << (offset + policy_count - 1) \
            << " ]: " << choice << std::endl;
    } else {
        choice = 0;
    }

    return choice;
}

Random::Random(int num_policies)
    : Model(num_policies, "Random", true)
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
#elif APOLLO_LOCAL_TRAINING
    offset = 0;
#else
#error "Missing training configuration"
#endif

    //std::cout << "rank " << rank << " policy_count " << policy_count \
        << " offset " << offset << std::endl; //ggout
}

Random::~Random()
{
    return;
}
