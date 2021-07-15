// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (MIT)

#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include "apollo/PolicyModel.h"

#include <string>

class Static : public PolicyModel {
    public:
      Static(int num_policies, int policy_choice)
          : PolicyModel(num_policies,
                        "Static," + std::to_string(policy_choice),
                        false),
            policy_choice(policy_choice){};
      ~Static(){};

      //
      int getIndex(std::vector<float> &features);
      void store(const std::string &filename){};

    private:
        int policy_choice;

}; //end: Static (class)


#endif
