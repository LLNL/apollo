

// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

#include "apollo/models/Optimal.h"

Optimal::Optimal(std::string file) : PolicyModel(0, "Optimal", false) {
    std::ifstream infile(file);
    std::string policy_str;

    while(std::getline(infile, policy_str, ','))
        optimal_policy.push_back(std::stoi(policy_str));

    infile.close();
}

int
Optimal::getIndex(std::vector<float> &features)
{
    if (optimal_policy.empty()) {
        std::cerr << "Optimal policy queue is empty!" << std::endl;
        abort();
    }

    int policy = optimal_policy.front();
    optimal_policy.pop_front();
    return policy;
}
