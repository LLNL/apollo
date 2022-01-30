// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include "apollo/models/RegressionTree.h"
#include "models/preprocessing/Preprocessing.h"

using namespace std;


RegressionTree::RegressionTree(
    std::vector<std::tuple<std::vector<float>, int, double> > &measures)
    : TimingModel("RegressionTree")
{
  std::vector<std::vector<float>> features;
  std::vector<int> responses;
  std::map<std::vector<float>, std::pair<int, double>> min_metric_policies;
  Preprocessing::findMinMetricPolicyByFeatures(measures,
                                               features,
                                               responses,
                                               min_metric_policies);
  // std::chrono::steady_clock::time_point t1, t2;
  // t1 = std::chrono::steady_clock::now();

  //
  // dtree = KNearest::create();
  //

  ////dtree = DTrees::create();
  dtree = RTrees::create();
  dtree->setMinSampleCount(1);
  dtree->setTermCriteria(
      TermCriteria(TermCriteria::MAX_ITER + TermCriteria::EPS, 50, 0.001));
  // dtree->setTermCriteria( TermCriteria(  TermCriteria::MAX_ITER, 50, 0.01 )
  // ); dtree->setMaxDepth(8);
  dtree->setRegressionAccuracy(1e-6);
  dtree->setUseSurrogates(false);
  dtree->setCVFolds(0);
  dtree->setUse1SERule(false);
  dtree->setTruncatePrunedTree(false);
  dtree->setPriors(Mat());

  Mat fmat;
  for (auto &i : features) {
    Mat tmp(1, i.size(), CV_32FC1, &i[0]);
    fmat.push_back(tmp);
    }

    Mat rmat;
    //rmat = Mat::zeros( fmat.rows, num_policies, CV_32F );
    //for( int i = 0; i < responses.size(); i++ ) {
    //    int j = responses[i];
    //    rmat.at<float>(i, j) = 1.f;
    //}
    Mat(fmat.rows, 1, CV_32F, &responses[0]).copyTo(rmat);

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
    //std::cout << "Train regression tree " << " : " << duration << " seconds" << std::endl;

    return;
}

RegressionTree::~RegressionTree()
{
    return;
}

double
RegressionTree::getTimePrediction(std::vector<float> &features)
{

    //std::chrono::steady_clock::time_point t1, t2;
    //t1 = std::chrono::steady_clock::now();
    // Keep choice around for easier debugging, if needed:
    double choice = 0;

    //Mat fmat;
    //fmat.push_back( Mat(1, features.size(), CV_32F, &features[0]) );
    //Mat result;
    //choice = dtree->predict( features, result );
    //std::cout << "Results: " << result << std::endl;
    //
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

void RegressionTree::store(const std::string &filename)
{
    dtree->save( filename );
}
