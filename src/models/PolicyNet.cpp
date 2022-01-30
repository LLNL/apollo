// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/PolicyNet.h"

#include <bitset>
#include <climits>
#include <fstream>
#include <iostream>
#include <numeric>

#include "helpers/OutputFormatter.h"
#include "helpers/TimeTrace.h"

class FCLayer
{
public:
  FCLayer(int inputSize, int outputSize)
      : inputSize(inputSize), outputSize(outputSize)
  {
    // Create the random number generator and the distribution for
    // He initialization.
    std::random_device rd;
    std::mt19937_64 generator(rd());
    double stddev = std::sqrt(2. / inputSize);
    std::normal_distribution<double> normal_dist(0., stddev);

    // Allocate and zero initialize the arrays needed for the parameters.
    weights = new double[outputSize * inputSize]();
    weights_m = new double[outputSize * inputSize]();
    weights_v = new double[outputSize * inputSize]();
    weights_grad = new double[outputSize * inputSize]();

    bias = new double[outputSize]();
    bias_m = new double[outputSize]();
    bias_v = new double[outputSize]();
    bias_grad = new double[outputSize]();

    // Randomly initializer the layer weights.
    for (int i = 0; i < outputSize; ++i) {
      for (int j = 0; j < inputSize; ++j)
        weights[i * inputSize + j] = normal_dist(generator);
      // TODO: does the bias help?
      // bias[i] = normal_dist(generator);
    }
  };

  ~FCLayer()
  {
    // Delete all allocated arrays.
    delete[] weights;
    delete[] weights_m;
    delete[] weights_v;
    delete[] weights_grad;

    delete[] bias;
    delete[] bias_m;
    delete[] bias_v;
    delete[] bias_grad;

    if (outputs != NULL) {
      delete[] outputs;
      delete[] inputGrad;
    }
  }

  double *forward(double *inputs, int batchSize)
  {
    // Check if the current batch size is larger than the maximum so far.
    if (batchSize > maxBatchSize) {
      // Delete the old arrays if they exist.
      if (outputs != NULL) {
        delete[] outputs;
        delete[] inputGrad;
      }

      // Allocate new arrays for the batch size.
      outputs = new double[batchSize * outputSize]();
      inputGrad = new double[batchSize * inputSize];

      maxBatchSize = batchSize;
    } else {
      // Zero the output array.
      for (int i = 0; i < batchSize * outputSize; ++i) {
        outputs[i] = 0;
      }
    }

    // Compute the forward pass of this layer: y = M x + b
    for (int i = 0; i < batchSize; ++i) {
      for (int j = 0; j < outputSize; ++j) {
        for (int k = 0; k < inputSize; ++k) {
          outputs[i * outputSize + j] +=
              weights[j * inputSize + k] * inputs[i * inputSize + k];
        }

        outputs[i * outputSize + j] += bias[j];
      }
    }

    return outputs;
  }

  double *backward(double *inputs, double *chainRuleGrad, int batchSize)
  {
    // Zero all gradient arrays.
    for (int i = 0; i < batchSize * inputSize; ++i) {
      inputGrad[i] = 0;
    }

    for (int i = 0; i < outputSize * inputSize; ++i) {
      weights_grad[i] = 0;
    }

    for (int i = 0; i < outputSize; ++i) {
      bias_grad[i] = 0;
    }

    // Compute the gradients using the chain rule with the gradients from the
    // previous layer.
    for (int i = 0; i < batchSize; ++i) {
      for (int j = 0; j < outputSize; ++j) {
        for (int k = 0; k < inputSize; ++k) {
          weights_grad[j * inputSize + k] +=
              chainRuleGrad[i * outputSize + j] * inputs[i * inputSize + k];
          inputGrad[i * inputSize + k] +=
              chainRuleGrad[i * outputSize + j] * weights[j * inputSize + k];
        }

        bias_grad[j] += chainRuleGrad[i * outputSize + j];
      }
    }

    return inputGrad;
  }

  void step(double learnRate,
            double beta1 = 0.5,
            double beta2 = 0.9,
            double epsilon = 1e-8)
  {
    stepNum++;

    // Update each parameter in the layer using Adam optimization.
    for (int i = 0; i < outputSize; ++i) {
      for (int j = 0; j < inputSize; ++j) {
        int index = i * inputSize + j;

        // Update the moving averages of the first and second moments of the
        // gradient.
        weights_m[index] =
            beta1 * weights_m[index] + (1 - beta1) * weights_grad[index];
        weights_v[index] = beta2 * weights_v[index] +
                           (1 - beta2) * std::pow(weights_grad[index], 2);

        // Compute the unbiased moving averages.
        double mHat = weights_m[index] / (1 - std::pow(beta1, stepNum));
        double vHat = weights_v[index] / (1 - std::pow(beta2, stepNum));

        // Update the weights.
        weights[index] += learnRate * mHat / (std::sqrt(vHat) + epsilon);
      }

      // Update the moving averages of the first and second moments of the
      // gradient.
      bias_m[i] = beta1 * bias_m[i] + (1 - beta1) * bias_grad[i];
      bias_v[i] = beta2 * bias_v[i] + (1 - beta2) * std::pow(bias_grad[i], 2);

      // Compute the unbiased moving averages.
      double mHat = bias_m[i] / (1 - std::pow(beta1, stepNum));
      double vHat = bias_v[i] / (1 - std::pow(beta2, stepNum));

      // Update the weights.
      bias[i] += learnRate * mHat / (std::sqrt(vHat) + epsilon);
    }
  }

  int inputSize;
  int outputSize;

  double *weights;
  double *bias;

private:
  long stepNum = 0;
  double *weights_m;
  double *weights_v;
  double *weights_grad;
  double *bias_m;
  double *bias_v;
  double *bias_grad;

  double *outputs = NULL;
  double *inputGrad;
  int maxBatchSize = 0;
};

class ReLU
{
public:
  ~ReLU()
  {
    // Delete allocated arrays.
    if (outputs != NULL) {
      delete[] outputs;
      delete[] inputGrad;
    }
  }

  double *forward(double *inputs, int batchSize, int outputSize)
  {
    // Check if the arrays need to be reallocated.
    if (batchSize * outputSize > maxArraySize) {
      maxArraySize = batchSize * outputSize;

      // Delete old arrays if they exist.
      if (outputs != NULL) {
        delete[] outputs;
        delete[] inputGrad;
      }

      outputs = new double[maxArraySize];
      inputGrad = new double[maxArraySize];
    }

    // Compute ReLU activation.
    for (int i = 0; i < batchSize * outputSize; ++i) {
      outputs[i] = std::max(inputs[i], 0.);
    }

    return outputs;
  }

  double *backward(double *inputs,
                   double *chainRuleGrad,
                   int batchSize,
                   int inputSize)
  {
    // Compute the gradients of the ReLU layer using the chain rule with the
    // gradients from the previous layer.
    for (int i = 0; i < batchSize * inputSize; ++i) {
      inputGrad[i] = chainRuleGrad[i] * std::signbit(-inputs[i]);
    }

    return inputGrad;
  }

private:
  double *outputs = NULL;
  double *inputGrad;
  int maxArraySize = 0;
};

class Softmax
{
public:
  ~Softmax()
  {
    // Delete allocated arrays.
    if (outputs != NULL) {
      delete[] outputs;
      delete[] inputGrad;
    }
  }

  double *forward(double *inputs, int batchSize, int outputSize)
  {
    // Check if the arrays need to be reallocated.
    if (batchSize * outputSize > maxArraySize) {
      maxArraySize = batchSize * outputSize;

      // Delete old arrays if they exist.
      if (outputs != NULL) {
        delete[] outputs;
        delete[] inputGrad;
      }

      outputs = new double[maxArraySize];
      inputGrad = new double[maxArraySize];
    }

    // Compute the maximum of each previous layer for each sample. This is used
    // to improve numerical stability.
    double alpha[batchSize];
    for (int i = 0; i < batchSize; ++i) {
      alpha[i] = inputs[i * outputSize];
      for (int j = 0; j < outputSize; ++j) {
        alpha[i] = std::max(alpha[i], inputs[i * outputSize + j]);
      }
    }

    // Calculate the probabilities for each output using softmax.
    for (int i = 0; i < batchSize; ++i) {
      double sum = 0;
      for (int j = 0; j < outputSize; ++j) {
        int index = i * outputSize + j;
        outputs[index] = std::exp(
            inputs[index] -
            alpha[i]);  // Note: subtract alpha for numerical stability.
        sum += outputs[index];
      }
      for (int j = 0; j < outputSize; ++j) {
        outputs[i * outputSize + j] /= sum;
      }
    }

    return outputs;
  }

  double *lossGrad(int *action,
                   double *actionProbs,
                   double *reward,
                   int batchSize,
                   int inputSize)
  {
    // Compute the gradient of the softmax layer for the given state, action,
    // and reward.
    for (int i = 0; i < batchSize; ++i) {
      for (int j = 0; j < inputSize; ++j) {
        int index = i * inputSize + j;
        inputGrad[index] = -reward[i] * actionProbs[index] / batchSize;
      }
      inputGrad[i * inputSize + action[i]] += reward[i] / batchSize;
    }

    return inputGrad;
  }

private:
  double *outputs = NULL;
  double *inputGrad;
  int maxArraySize = 0;
};

class Net
{
public:
  Net(int inputSize,
      int hiddenSize,
      int outputSize,
      double learnRate = 1e-2,
      double beta1 = 0.5,
      double beta2 = 0.9)
      : inputSize(inputSize),
        hiddenSize(hiddenSize),
        outputSize(outputSize),
        learnRate(learnRate),
        beta1(beta1),
        beta2(beta2),
        layer1(FCLayer(inputSize, hiddenSize)),
        layer2(FCLayer(hiddenSize, hiddenSize)),
        layer3(FCLayer(hiddenSize, outputSize))
  {
  }

  double *forward(double *state, int batchSize)
  {
    // Compute the forward pass through each layer of the network.
    auto out1 = layer1.forward(state, batchSize);
    auto aout1 = relu1.forward(out1, batchSize, hiddenSize);
    auto out2 = layer2.forward(aout1, batchSize);
    auto aout2 = relu2.forward(out2, batchSize, hiddenSize);
    auto out3 = layer3.forward(aout2, batchSize);
    auto aout3 = softmax.forward(out3, batchSize, outputSize);

    return aout3;
  }

  void trainStep(double *state, int *action, double *reward, int batchSize)
  {
    // Compute the forward pass through each layer of the network.
    auto out1 = layer1.forward(state, batchSize);
    auto aout1 = relu1.forward(out1, batchSize, hiddenSize);
    auto out2 = layer2.forward(aout1, batchSize);
    auto aout2 = relu2.forward(out2, batchSize, hiddenSize);
    auto out3 = layer3.forward(aout2, batchSize);
    auto aout3 = softmax.forward(out3, batchSize, outputSize);

    // Compute the backward pass through each layer and compute the gradients.
    auto aout3_grad =
        softmax.lossGrad(action, aout3, reward, batchSize, outputSize);
    auto out3_grad = layer3.backward(aout2, aout3_grad, batchSize);
    auto aout2_grad = relu2.backward(out2, out3_grad, batchSize, hiddenSize);
    auto out2_grad = layer2.backward(aout1, aout2_grad, batchSize);
    auto aout1_grad = relu1.backward(out1, out2_grad, batchSize, hiddenSize);
    layer1.backward(state, aout1_grad, batchSize);

    // Update the parameters of each layer.
    layer1.step(learnRate, beta1, beta2);
    layer2.step(learnRate, beta1, beta2);
    layer3.step(learnRate, beta1, beta2);
  }

  FCLayer layer1;
  FCLayer layer2;
  FCLayer layer3;

private:
  int inputSize;
  int hiddenSize;
  int outputSize;
  double learnRate;
  double beta1;
  double beta2;
  ReLU relu1;
  ReLU relu2;
  Softmax softmax;
};

PolicyNet::PolicyNet(int numPolicies,
                     int numFeatures,
                     double lr = 1e-2,
                     double beta = 0.5,
                     double beta1 = 0.5,
                     double beta2 = 0.9,
                     double threshold = 0.0)
    : PolicyModel(numPolicies, "PolicyNet"),
      numPolicies(numPolicies),
      beta(beta),
      threshold(threshold)
{
  // Encode features by bit-representation, assumes 64 bits are enough.
  net = std::make_unique<Net>(64 * numFeatures,
                              (64 * numFeatures + numPolicies) / 2,
                              numPolicies,
                              lr,
                              beta1,
                              beta2);
  // Seed the random number generator using the current time.
  std::random_device rd;
  gen.seed(rd());
}

PolicyNet::~PolicyNet() {}

void PolicyNet::train(
    std::vector<std::tuple<std::vector<float>, int, double>> &measures)
{
  std::vector<std::vector<float>> states;
  std::vector<int> actions;
  std::vector<double> rewards;

  for (auto &measure : measures) {
    auto &features = std::get<0>(measure);
    auto policy = std::get<1>(measure);
    auto metric = std::get<2>(measure);

    // XXX: assumes features can be casted to 64-bit unsigned integers.
    std::bitset<sizeof(uint64_t) * CHAR_BIT> bits((uint64_t)features[0]);
    std::vector<float> features_bits;
    for (int i = 0; i < bits.size(); i++)
      features_bits.push_back(bits[i]);
    states.push_back(std::move(features_bits));
    actions.push_back(policy);
    // XXX: metric is assumed to be execution time, hence reward should maximize
    // when execution time is minimized. Out of various alternatives, e^-metric
    // normalizes within (0, 1) and rewards least execution time.
    // rewards.push_back(-metric);
    // rewards.push_back(1.0 / metric);
    rewards.push_back(std::exp(-metric));
  }

  trainNet(states, actions, rewards);

  actionProbabilityMap.clear();
}

void PolicyNet::trainNet(std::vector<std::vector<float>> &states,
                         std::vector<int> &actions,
                         std::vector<double> &rewards)
{
  int batchSize = states.size();
  if (batchSize < 1) return;  // Don't train if there is no data to train on.
  int inputSize = states[0].size();

  // Calculate the average reward of the batch.
  double batchRewardAvg = std::accumulate(rewards.begin(), rewards.end(), 0.0);
  batchRewardAvg /= batchSize;

  // Update the moving average of the reward.
  rewardMovingAvg = beta * rewardMovingAvg + (1 - beta) * batchRewardAvg;

  // Debias the estimate of the moving average.
  double baseline = rewardMovingAvg / (1 - std::pow(beta, ++trainCount));

  // Don't train if the average execution time is less than the threshold.
  if (baseline < threshold) return;

  // Create the arrays used for training.
  double *trainStates = new double[batchSize * inputSize];
  int *trainActions = new int[batchSize];
  double *trainRewards = new double[batchSize];

  // Fill the arrays used for training.
  for (int i = 0; i < batchSize; ++i) {
    for (int j = 0; j < inputSize; ++j)
      trainStates[i * inputSize + j] = states[i][j];

    trainActions[i] = actions[i];
    trainRewards[i] =
        rewards[i] -
        baseline;  // Subtract the moving average baseline to reduce variance.
    //std::cout << "=====\n;";
    //std::cout << "TRAINING DATA i " << i << "\n";
    //std::cout << "trainState [ ";
    //for (int j = 0; j < inputSize; ++j)
    //  std::cout << trainStates[i * inputSize + j] << ", ";
    //std::cout << " ]\n";
    //std::cout << "trainActions " << trainActions[i] << "\n";
    //std::cout << "trainRewards " << trainRewards[i] << "\n";
    //std::cout << "=====\n";
  }

  // Train the network.
  // for(int i = 0; i < batchSize; i+=64)
  //  net->trainStep(&trainStates[i*inputSize], &trainActions[i],
  //  &trainRewards[i], 64);
  net->trainStep(trainStates, trainActions, trainRewards, batchSize);

  // Delete the arrays used for training.
  delete[] trainStates;
  delete[] trainActions;
  delete[] trainRewards;
}

int PolicyNet::getIndex(std::vector<float> &features)
{
  // Debias the estimate of the moving average.
  double baseline =
      (trainCount > 0 ? rewardMovingAvg / (1 - std::pow(beta, trainCount))
                      : 0.0);

  // Don't evaluate if the average execution time is less than the threshold, by
  // convention return policy 0 as the default.
  if (baseline < threshold) return 0;

  std::vector<double> actionProbs;

  // Check if these features have already been evaluated since the previous
  // network update.
  std::string key = "";
  int inputSize = features.size() * 64;
  std::bitset<sizeof(uint64_t) * CHAR_BIT> bits;
  for (auto &feature : features) {
    bits = (uint64_t)feature;
    key.append(bits.to_string());
  }

  auto it = actionProbabilityMap.find(key);
  if (it != actionProbabilityMap.end()) {
    // Use the previously evaluated action probabilities.
    actionProbs = it->second;
  } else {
    // Create the state array to be evaluated by the network.
    double *evalState = new double[inputSize];
    for (int i = 0; i < features.size(); i++) {
      size_t index = i * inputSize;
      std::bitset<sizeof(uint64_t) * CHAR_BIT> bits((uint64_t)features[i]);
      for (int j = 0; j < inputSize; ++j) {
        evalState[index + j] = bits[j];
      }
    }

    // Compute the action probabilities using the network and store in a vector.
    double *evalActionProbs = net->forward(evalState, 1);
    actionProbs = std::vector<double>(numPolicies);
    for (int i = 0; i < numPolicies; ++i)
      actionProbs[i] = evalActionProbs[i];

    // Delete the state array.
    delete[] evalState;

    // Add the action probabilities to the cache to be reused later.
    actionProbabilityMap.insert(std::make_pair(key, actionProbs));
  }
  //std::cout << " probs: ";
  //for (auto &p : actionProbs) {
  //  std::cout << p << " ";
  //}
  //std::cout << std::endl;

  // Sample a policy from the action probabilities.
  std::discrete_distribution<> d(actionProbs.begin(), actionProbs.end());
  int policyIndex = d(gen);

  return policyIndex;
}

void PolicyNet::store(const std::string &filename)
{
#if 0
  // TODO: The human-readable yaml format has little value since weights are not
  // interpretable. However, when we make the NN generic a human-readable yaml
  // will have value to inspect the architecture of the network.
  std::ofstream ofs(filename, std::ofstream::out);
  OutputFormatter outfmt(ofs);

  auto output_layer = [&outfmt](FCLayer &layer) {
    outfmt << "layer:\n";
    ++outfmt;
    outfmt << "weights: [";
    for (int i = 0; i < layer.inputSize * layer.outputSize; ++i) {
      if (i % 8 == 0) outfmt << "\n";
      outfmt & layer.weights[i] & ", ";
    }
    outfmt << "\n]\n";
    --outfmt;
  };

  outfmt << "# PolicyNet\n";
  outfmt << "policynet {\n";
  ++outfmt;
  output_layer(net->layer1);
  --outfmt;
  outfmt << "}\n";
  ofs.close();
  return;
#endif

  // Open the output file in binary write mode.
  std::ofstream f(filename, std::ios::binary);

  // Check if the file was opened successfully.
  if (!f) {
    std::cerr << "Could not save model to " << filename << std::endl;
    return;
  }

  // Write the weights and biases of each layer to the output file.
  f.write((char *)net->layer1.weights,
          sizeof(double) * net->layer1.inputSize * net->layer1.outputSize);
  f.write((char *)net->layer1.bias, sizeof(double) * net->layer1.outputSize);
  f.write((char *)net->layer2.weights,
          sizeof(double) * net->layer2.inputSize * net->layer2.outputSize);
  f.write((char *)net->layer2.bias, sizeof(double) * net->layer2.outputSize);
  f.write((char *)net->layer3.weights,
          sizeof(double) * net->layer3.inputSize * net->layer3.outputSize);
  f.write((char *)net->layer3.bias, sizeof(double) * net->layer3.outputSize);

  // Store reward moving average so that the threshold still works if the model
  // is loaded without retraining.
  f.write((char *)&rewardMovingAvg, sizeof(double));
  f.write((char *)&trainCount, sizeof(int));

  f.close();
}

void PolicyNet::load(const std::string &filename)
{
  // Open the save file in binary read mode.
  std::ifstream f(filename, std::ios::binary);

  // Check if the file was opened successfully.
  if (!f) {
    std::cerr << "Could not load model from " << filename << std::endl;
    return;
  }

  // Load the weights and biases of each layer from the save file.
  f.read((char *)net->layer1.weights,
         sizeof(double) * net->layer1.inputSize * net->layer1.outputSize);
  f.read((char *)net->layer1.bias, sizeof(double) * net->layer1.outputSize);
  f.read((char *)net->layer2.weights,
         sizeof(double) * net->layer2.inputSize * net->layer2.outputSize);
  f.read((char *)net->layer2.bias, sizeof(double) * net->layer2.outputSize);
  f.read((char *)net->layer3.weights,
         sizeof(double) * net->layer3.inputSize * net->layer3.outputSize);
  f.read((char *)net->layer3.bias, sizeof(double) * net->layer3.outputSize);

  // Load reward moving average so that the threshold still works if the model
  // is loaded without retraining.
  f.read((char *)&rewardMovingAvg, sizeof(double));
  f.read((char *)&trainCount, sizeof(int));

  f.close();
}
