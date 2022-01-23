#ifndef APOLLO_MODEL_FACTORY_H
#define APOLLO_MODEL_FACTORY_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "apollo/PolicyModel.h"
#include "apollo/TimingModel.h"

// Factory
class ModelFactory
{
public:
  static std::unique_ptr<PolicyModel> createStatic(int num_policies,
                                                   int policy_choice);
  static std::unique_ptr<PolicyModel> createRandom(int num_policies);
  static std::unique_ptr<PolicyModel> createRoundRobin(int num_policies);

  static std::unique_ptr<PolicyModel> createTuningModel(
      const std::string &model_name,
      int num_policies,
      const std::string &path);
  static std::unique_ptr<PolicyModel> createTuningModel(
      const std::string &model_name,
      int num_policies,
      std::vector<std::tuple<std::vector<float>, int, double> > &measures,
      std::unordered_map<std::string, std::string> &model_params);
  static std::unique_ptr<PolicyModel> createOptimal(std::string file);

  static std::unique_ptr<TimingModel> createRegressionTree(
      std::vector<std::tuple<std::vector<float>, int, double> > &measures);
};  // end: ModelFactory


#endif
