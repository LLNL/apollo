// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <string>
#include <vector>
#include <map>

#include "apollo/PolicyModel.h"

class RoundRobin : public PolicyModel {
    public:
        RoundRobin(int num_policies);
        ~RoundRobin();

        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename) {};

    private:
        std::map< std::vector<float>, int > policies;
        int last_policy;

}; //end: RoundRobin (class)


#endif
