
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
#include <mutex>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/models/Sequential.h"

#define modelName "sequential"
#define modelFile __FILE__

int
Apollo::Model::Sequential::getIndex(void)
{
    iter_count++;

    static int choice = -1;

    // Return a sequential index, 0..N:
    choice++;

    if (choice == policy_count) {
        choice = 0;
    }

    return choice;
}

void
Apollo::Model::Sequential::configure(
        int  num_policies,
        json new_model_def)
{
    apollo        = Apollo::instance();
    policy_count  = num_policies;
    model_def     = new_model_def;
    configured    = true;
    return;
}

//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Sequential::Sequential()
{
    name = "Sequential";
    iter_count = 0;
}

Apollo::Model::Sequential::~Sequential()
{
    return;
}

extern "C" Apollo::Model::Sequential*
APOLLO_model_create_sequential(void)
{
    return new Apollo::Model::Sequential;
}


extern "C" void
APOLLO_model_destroy_sequential(
        Apollo::Model::Sequential *model_ref)
{
    delete model_ref;
    return;
}


