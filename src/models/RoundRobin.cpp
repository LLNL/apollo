
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
#include <cstring>

#include "apollo/models/RoundRobin.h"

#define modelName "roundrobin"
#define modelFile __FILE__

int RoundRobin::policy_choice = -1;

int
RoundRobin::getIndex(std::vector<float> &features)
{
    int choice = RoundRobin::policy_choice;

    RoundRobin::policy_choice = ( RoundRobin::policy_choice + 1 ) % policy_count;

    return choice;
}

RoundRobin::RoundRobin(
        int   num_policies)
    : Model(num_policies, "RoundRobin", true)
{
    policy_count  = num_policies;

    int rank = 0;
    char *slurm_procid = getenv("SLURM_PROCID");

    if (slurm_procid != NULL) {
       rank = atoi(slurm_procid);
    } else {
       rank = 0;
    };

    if( RoundRobin::policy_choice < 0 )
        RoundRobin::policy_choice = rank % policy_count;

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
