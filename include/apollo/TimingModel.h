// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_TIMING_MODEL_H
#define APOLLO_TIMING_MODEL_H

#include <string>
#include <vector>

#include "apollo/Dataset.h"

// Abstract
class TimingModel {
    public:
        TimingModel(std::string name) : name(name) {};
        virtual ~TimingModel() {}
        virtual double getTimePrediction(std::vector<float> &features) = 0;
        virtual void store(const std::string &filename) = 0;

        std::string      name           = "";
}; //end: TimingModel (abstract class)


#endif
