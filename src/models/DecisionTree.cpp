
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


#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include "apollo/models/DecisionTree.h"

#define modelName "decisiontree"
#define modelFile __FILE__


using namespace std;

#include <opencv2/ml.hpp>
using namespace cv;
using namespace cv::ml;

static Ptr<DTrees> dtree;

DecisionTree::DecisionTree(int num_policies, std::vector< std::vector<float> > &features, std::vector<int> &responses)
    : Model(num_policies, "DecisionTree", false)
{

    //std::chrono::steady_clock::time_point t1, t2;
    //t1 = std::chrono::steady_clock::now();

    dtree = DTrees::create();
    dtree->setMaxDepth(2);
    dtree->setMinSampleCount(2);
    dtree->setRegressionAccuracy(0);
    dtree->setUseSurrogates(false);
    dtree->setMaxCategories(policy_count);
    dtree->setCVFolds(0);
    dtree->setUse1SERule(false);
    dtree->setTruncatePrunedTree(false);
    //dtree->setPriors(Mat());

    Mat fmat;
    for(auto &i : features) {
        Mat tmp(1, i.size(), CV_32F, &i[0]);
        fmat.push_back(tmp);
    }

    Mat rmat;
    Mat(fmat.rows, 1, CV_32S, &responses[0]).copyTo(rmat);

    //std::cout << "fmat: " << fmat << std::endl;
    //std::cout << "features.size: " << features.size() << std::endl;
    //std::cout << "rmat: " << rmat << std::endl;

    dtree->train(fmat, ROW_SAMPLE, rmat);

    //t2 = std::chrono::steady_clock::now();
    //double duration = std::chrono::duration<double>(t2 - t1).count();
    //std::cout << "Train " << name<< " : " << duration << " seconds" << std::endl;

    return;
}

DecisionTree::~DecisionTree()
{
    return;
}

int
DecisionTree::getIndex(std::vector<float> &features)
{

    //std::chrono::steady_clock::time_point t1, t2;
    //t1 = std::chrono::steady_clock::now();
    // Keep choice around for easier debugging, if needed:
    static int choice = -1;


    choice = dtree->predict( features );

    //t2 = std::chrono::steady_clock::now();
    //double duration = std::chrono::duration<double>(t2 - t1).count();
    //std::cout << "predict duration: " <<  duration << " choice: " << choice << endl; //ggout
    //std::cout << "Features: [ ";
    //for(auto &i: features) 
    //    std::cout << i;
    //std::cout << " ] ->  " << choice << std::endl;

    // Find the recommendation of the decision tree:

    return choice;
}

void DecisionTree::store(const std::string &filename)
{
    dtree->save( filename );
}
