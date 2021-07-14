// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <iostream> //ggout

#include "apollo/Config.h"
#include "apollo/models/Random.h"

int
Random::getIndex(std::vector<float> &features)
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

Random::Random(int num_policies)
    : PolicyModel(num_policies, "Random", true)
{
    int rank = 0;
    char *slurm_procid = getenv("SLURM_PROCID");

    if (slurm_procid != NULL) {
        rank = atoi(slurm_procid);
    } else {
        rank = 0;
    };

    random_gen = std::mt19937( random_dev() );
    random_dist = std::uniform_int_distribution<>( 0, policy_count - 1 );
}

Random::~Random()
{
    return;
}
