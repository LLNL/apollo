
// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (MIT)


#include <string>

#include "apollo/models/Sequential.h"

int
Sequential::getIndex(std::vector<float> &features)
{

    static int choice = -1;

    // Return a sequential index, 0..N:
    choice++;

    if (choice == policy_count) {
        choice = 0;
    }

    return choice;
}

Sequential::Sequential(int num_policies)
    : PolicyModel(num_policies, "Sequential", true)
{
}

Sequential::~Sequential()
{
    return;
}
