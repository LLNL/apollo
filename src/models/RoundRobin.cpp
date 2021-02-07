
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
