// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#ifndef APOLLO_MODELS_REGRESSIONTREE_H
#define APOLLO_MODELS_REGRESSIONTREE_H

#include <string>
#include <vector>

#include <opencv2/ml.hpp>
using namespace cv;
using namespace cv::ml;

#include "apollo/TimingModel.h"

class RegressionTree : public TimingModel {

    public:
        RegressionTree(std::vector< std::vector<float> > &features, std::vector<float> &responses);

        ~RegressionTree();

        double getTimePrediction(std::vector<float> &features);
        void store(const std::string &filename);

    private:
        // Ptr<DTrees> dtree;
        Ptr<RTrees> dtree;
        //Ptr<KNearest> dtree;
        //Ptr<Boost> dtree;
        //Ptr<ANN_MLP> dtree;
        //Ptr<LogisticRegression> dtree;
}; //end: RegressionTree (class)


#endif
