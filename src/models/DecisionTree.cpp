
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
#include <opencv2/core/types.hpp>

#define modelName "decisiontree"
#define modelFile __FILE__


using namespace std;

DecisionTree::DecisionTree(int num_policies, std::vector< std::vector<float> > &features, std::vector<int> &responses)
    : PolicyModel(num_policies, "DecisionTree", false)
{

    //std::chrono::steady_clock::time_point t1, t2;
    //t1 = std::chrono::steady_clock::now();

    //dtree = NormalBayesClassifier::create();
    //
    //dtree = KNearest::create();
    //
    //dtree = Boost::create();
    //
    //dtree = ANN_MLP::create();
    //
    //dtree = SVM::create();
    //dtree = LogisticRegression::create();
    //dtree->setLearningRate(0.001);
    //dtree->setIterations(10);
    //dtree->setRegularization(LogisticRegression::REG_L2);
    //dtree->setTrainMethod(LogisticRegression::BATCH);
    //dtree->setMiniBatchSize(1);

    //dtree = DTrees::create();
    dtree = RTrees::create();
    dtree->setTermCriteria( TermCriteria(  TermCriteria::MAX_ITER + TermCriteria::EPS, 10, 0.01 ) );
    //dtree->setTermCriteria( TermCriteria(  TermCriteria::MAX_ITER, 10, 0 ) );

    dtree->setMaxDepth(2);

    dtree->setMinSampleCount(2);
    dtree->setRegressionAccuracy(0);
    dtree->setUseSurrogates(false);
    dtree->setMaxCategories(policy_count);
    dtree->setCVFolds(0);
    dtree->setUse1SERule(false);
    dtree->setTruncatePrunedTree(false);
    dtree->setPriors(Mat());

    Mat fmat;
    for(auto &i : features) {
        Mat tmp(1, i.size(), CV_32F, &i[0]);
        fmat.push_back(tmp);
    }

    Mat rmat;
    //rmat = Mat::zeros( fmat.rows, num_policies, CV_32F );
    //for( int i = 0; i < responses.size(); i++ ) {
    //    int j = responses[i];
    //    rmat.at<float>(i, j) = 1.f;
    //}
    Mat(fmat.rows, 1, CV_32S, &responses[0]).copyTo(rmat);
    //Mat(fmat.rows, 1, CV_32F, &responses[0]).copyTo(rmat);

    //std::cout << "fmat: " << fmat << std::endl;
    //std::cout << "features.size: " << features.size() << std::endl;
    //std::cout << "rmat: " << rmat << std::endl;

    // ANN_MLP
    //dtree->setActivationFunction(ANN_MLP::ActivationFunctions::SIGMOID_SYM);
    //Mat layers(3, 1, CV_16U);
    //layers.row(0) = Scalar(fmat.cols);
    //layers.row(1) = Scalar(4);
    //layers.row(2) = Scalar(rmat.cols);
    //dtree->setLayerSizes( layers );
    //dtree->setTrainMethod(ANN_MLP::TrainingMethods::BACKPROP);

    dtree->train(fmat, ROW_SAMPLE, rmat);
    //Ptr<TrainData> data = TrainData::create(fmat, ROW_SAMPLE, rmat);
    //dtree->train(data);
    //for(int i = 0; i<1000; i++)
    //    dtree->train(data, ANN_MLP::TrainFlags::UPDATE_WEIGHTS);

    //if(!dtree->isTrained()) {
    //    std::cout << "MODEL IS NOT TRAINED!" << std::endl;
    //    abort();
    //}


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

    int choice = dtree->predict( features );

    //t2 = std::chrono::steady_clock::now();
    //double duration = std::chrono::duration<double>(t2 - t1).count();
    //std::cout << "DTree predict: " <<  duration << " choice: " << choice << endl; //ggout

    return choice;
}

void DecisionTree::store(const std::string &filename)
{
    dtree->save( filename );
}
