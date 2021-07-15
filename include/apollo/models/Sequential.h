// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (MIT)

#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#include <string>

#include "apollo/PolicyModel.h"

class Sequential : public PolicyModel {
    public:
        Sequential(int num_policies);
        ~Sequential();

        int  getIndex(std::vector<float> &features);

    private:

}; //end: Sequential (class)


#endif
