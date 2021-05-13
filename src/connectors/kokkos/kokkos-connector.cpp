#include <algorithm>
#include <apollo/Apollo.h>
#include <apollo/ModelFactory.h>
#include <apollo/Region.h>
#include <cassert>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <functional>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <utility>
#include <vector>

using namespace Kokkos::Tools::Experimental;

constexpr const int64_t slice_continuous = 100;
constexpr const double epsilon = 1E-24;
int flush_interval = 500;
using valunion =
    decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().value);
using valtype = decltype(
    std::declval<Kokkos::Tools::Experimental::VariableValue>().metadata->type);

struct VariableDatabaseData {
  int64_t canonical_id;
  char *name;
  int64_t candidate_set_size;
  valunion minimum;
  valunion maximum;
  Kokkos::Tools::Experimental::VariableValue *candidate_values;
};
int64_t slice_helper(double upper, double lower, double step) {
  return ((upper - lower) / step);
}
static size_t num_unconverged_regions;

int64_t count_range_slices(Kokkos::Tools::Experimental::ValueRange &in,
                           Kokkos::Tools::Experimental::ValueType type) {
  switch (type) {
  case ValueType::kokkos_value_int64:
    return 1 + ((((!in.openUpper) ? in.upper.int_value : (in.upper.int_value - 1)) -
            ((!in.openLower) ? in.lower.int_value : (in.lower.int_value + 1))) /
           in.step.int_value);
  case ValueType::kokkos_value_double:
    return (in.step.double_value ==
            0.0) // intentional comparison to double value 0.0, this shouldn't
                 // be a calculated value, but a value that has been set to the
                 // literal 0.0
               ? slice_continuous
               : slice_helper(in.openUpper ? in.upper.double_value
                                           : (in.upper.double_value - epsilon),
                              in.openLower ? in.lower.double_value
                                           : (in.lower.double_value + epsilon),
                              in.step.double_value);
  case ValueType::kokkos_value_string:
    // TODO: error mechanism?
    return -1;
  }
  return -1;
}

Kokkos::Tools::Experimental::VariableValue
mvv(size_t id, Kokkos_Tools_VariableValue_ValueUnionSet values,
    Kokkos::Tools::Experimental::VariableInfo *info, int index) {
  Kokkos_Tools_VariableValue_ValueUnion holder;
  switch (info->type) {
  case ValueType::kokkos_value_int64:
    holder.int_value = values.int_value[index];
    break;
  case ValueType::kokkos_value_double:
    holder.double_value = values.double_value[index];
    break;
  case ValueType::kokkos_value_string:
    strncpy(holder.string_value, values.string_value[index], 64);
    break;
  }
  Kokkos::Tools::Experimental::VariableValue value;
  value.type_id = id;
  value.value = holder;
  Kokkos::Tools::Experimental::VariableInfo *new_info =
      new Kokkos::Tools::Experimental::VariableInfo(*info);
  value.metadata = new_info;
  return value;
}

Kokkos::Tools::Experimental::VariableValue
mvv(size_t id, Kokkos_Tools_VariableValue_ValueUnion value,
    Kokkos::Tools::Experimental::VariableInfo *info) {
  Kokkos_Tools_VariableValue_ValueUnion holder;
  switch (info->type) {
  case ValueType::kokkos_value_int64:
    holder.int_value = value.int_value;
    break;
  case ValueType::kokkos_value_double:
    holder.double_value = value.double_value;
    break;
  case ValueType::kokkos_value_string:
    strncpy(holder.string_value, value.string_value, 64);
    break;
  }
  Kokkos::Tools::Experimental::VariableValue ret_value;
  ret_value.type_id = id;
  ret_value.value = holder;
  ret_value.metadata = info;
  return ret_value;
}
void form_set_from_range(Kokkos::Tools::Experimental::VariableValue *fill_this,
                         Kokkos_Tools_ValueRange with_this, int64_t num_slices,
                         size_t id,
                         Kokkos::Tools::Experimental::VariableInfo *info) {
  if (info->type == ValueType::kokkos_value_int64) {
    for (int x = 0; x < num_slices; ++x) {
      auto lower =
          (with_this.openLower
               ? with_this.lower.int_value
               : (with_this.lower.int_value + with_this.step.int_value));
      Kokkos_Tools_VariableValue_ValueUnion value;
      value.int_value = lower + (x * with_this.step.int_value);
      fill_this[x] = mvv(id, value, info);
    }
  } else if (info->type == ValueType::kokkos_value_double) {
    for (int x = 0; x < num_slices; ++x) {
      auto lower =
          (with_this.openLower ? with_this.lower.double_value
                               : (with_this.lower.double_value + epsilon));
      Kokkos_Tools_VariableValue_ValueUnion value;
      value.double_value = lower + (x * with_this.step.double_value);
      fill_this[x] = mvv(id, value, info);
    }
  }
}
void associate_candidates(const size_t id,
                          Kokkos::Tools::Experimental::VariableInfo *info) {
  int64_t candidate_set_size = 0;
  auto *databaseInfo =
      reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo);
  switch (info->valueQuantity) {
  case Kokkos::Tools::Experimental::CandidateValueType::kokkos_value_set:
    candidate_set_size = info->candidates.set.size;
    break;
  case CandidateValueType::kokkos_value_range:
    candidate_set_size = count_range_slices(info->candidates.range, info->type);
    break;
  case CandidateValueType::kokkos_value_unbounded:
    candidate_set_size = 0;
    break;
  }
  if (candidate_set_size > 0) {
    Kokkos::Tools::Experimental::VariableValue *candidate_values =
        new Kokkos::Tools::Experimental::VariableValue[candidate_set_size];
    switch (info->valueQuantity) {
    case CandidateValueType::kokkos_value_set:
      for (int x = 0; x < candidate_set_size; ++x) {
        candidate_values[x] = mvv(id, info->candidates.set.values, info, x);
      }
      break;
    case CandidateValueType::kokkos_value_range:
      form_set_from_range(candidate_values, info->candidates.range,
                          candidate_set_size, id, info);
      break;
    case CandidateValueType::kokkos_value_unbounded:
      candidate_set_size = 0;
      break;
    }
    databaseInfo->candidate_values = candidate_values;
  }
  databaseInfo->candidate_set_size = candidate_set_size;
}

std::map<std::string, Apollo::Region> regions;
std::set<size_t> untunables;
std::string activeRegion = "";
constexpr const int max_choices = 2 << 20;
constexpr const int max_variables = 50;

struct variableSet {
  Kokkos::Tools::Experimental::VariableValue variable_ids[max_variables];
  size_t num_variables;
};

int choices[max_choices];

namespace std {
template <> struct less<variableSet> {
  bool operator() (const variableSet &l, const variableSet &r) const {
    if (l.num_variables != r.num_variables) {
      return l.num_variables < r.num_variables;
    }
    for (int x = 0; x < l.num_variables; ++x) {
      if (l.variable_ids[x].type_id != r.variable_ids[x].type_id) {
        return l.variable_ids[x].type_id < r.variable_ids[x].type_id;
      }
    }
    return false;
  }
};
} // namespace std

struct options {
  bool disable_retrain;
  bool distributed_training;
  bool trace;
};

extern "C" void kokkosp_parse_args(int argc, char **argv) {
  options parsed_options;
  for (int x = 0; x < argc; ++x) {
    std::string arg(argv[x]);
    if (arg == "--disable_retrain") {
      parsed_options.disable_retrain = true;
      continue;
    }
    if (arg == "--distributed-training") {
      parsed_options.distributed_training = true;
      continue;
    }
    if (arg == "--trace") {
      parsed_options.trace = true;
      continue;
    }
    auto iter = arg.find("--flush-rate=");
    if (iter != std::string::npos) {
      flush_interval = atoi(arg.substr(arg.find("=") + 1).c_str());

      continue;
    }
  }

  std::map<std::string, std::string> settings;

  settings["APOLLO_RETRAIN_ENABLE="] =
      std::string((parsed_options.disable_retrain ? "0" : "1"));
  settings["APOLLO_COLLECTIVE_TRAINING="] =
      std::string((parsed_options.distributed_training ? "1" : "0"));
  settings["APOLLO_LOCAL_TRAINING="] =
      std::string((parsed_options.distributed_training ? "0" : "1"));
  settings["APOLLO_TRACE_BEST_POLICIES="] =
      std::string((parsed_options.trace ? "1" : "0"));
  settings["APOLLO_STORE_MODELS"] = "1";
  settings["APOLLO_INIT_MODEL"] = "RoundRobin";
  settings["APOLLO_REGION_MODEL"] = "1";
  for (const auto &kv : settings) {
    setenv(kv.first.c_str(), kv.second.c_str(), true);
  }
}

extern "C" void kokkosp_print_help(char *) {
  std::string OPTIONS_BLOCK =
      R"(
Apollo Connector for Kokkos, supported options

--disable-retrain      : tells Apollo not to retrain after convergence
--distributed_training : allows Apollo to distribute training tasks across MPI ranks
                           (requires MPI-enabled Apollo)
--trace                : tells Apollo to trace its result
--flush-rate=[int]     : tells Apollo how often to flush results
)";

  std::cout << OPTIONS_BLOCK;
}

Apollo *apollo;
extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  printf("Initializing Apollo Tuning adapter\n");
  for (int x = 0; x < max_choices; ++x) {
    choices[x] = x;
  }
  putenv("APOLLO_STORE_MODELS=1");
  putenv("APOLLO_RETRAIN_ENABLE=0");
  putenv("APOLLO_REGION_MODEL=1");
  putenv("APOLLO_LOCAL_TRAINING=1");
  putenv("APOLLO_INIT_MODEL=RoundRobin");
  putenv("APOLLO_COLLECTIVE_TRAINING=0");
  putenv("APOLLO_TRACE_BEST_POLICIES=1");
  apollo = Apollo::instance();
}

using RegionData = std::pair<std::string, Apollo::Region *>;

static std::map<size_t, std::pair<Apollo::Region *, Apollo::RegionContext *>>
    tuned_contexts;
static std::map<variableSet, RegionData> tuning_regions;
extern "C" void kokkosp_finalize_library() {
  printf("Finalizing Apollo Tuning adapter\n");
  for (auto kv : tuning_regions) {
    RegionData data = kv.second;
    auto file_name = data.first;
    auto *region = data.second;
  }
}
Kokkos::Tools::Experimental::ToolProgrammingInterface helper_functions;
void invoke_fence(uint32_t devID) {
  if ((helper_functions.fence != nullptr) && (num_unconverged_regions > 0)) {
    helper_functions.fence(devID);
  }
}
extern "C" void kokkosp_provide_tool_programming_interface(
    uint32_t num_actions,
    Kokkos::Tools::Experimental::ToolProgrammingInterface action) {
  helper_functions = action;
}

extern "C" void kokkosp_request_tool_settings(
    uint32_t num_responses,
    Kokkos::Tools::Experimental::ToolSettings *response) {
  response->requires_global_fencing = false;
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  invoke_fence(devID);
  *kID = devID;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  uint32_t devID = kID;
  invoke_fence(devID);
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  invoke_fence(devID);
  *kID = devID;
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  uint32_t devID = kID;
  invoke_fence(devID);
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  invoke_fence(devID);
  *kID = devID;
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  uint32_t devID = kID;
  invoke_fence(devID);
}

extern "C" void
kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo *info) {
  auto *metadata = new VariableDatabaseData();
  metadata->name = (char *)malloc(sizeof(char) * 65);
  strncpy(metadata->name, name, 64);
  info->toolProvidedInfo = metadata;
  associate_candidates(id, info);
}

extern "C" void
kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo *info) {
  auto *metadata = new VariableDatabaseData();
  metadata->name = (char *)malloc(sizeof(char) * 65);
  strncpy(metadata->name, name, 64);
  info->toolProvidedInfo = metadata;
}

float variableToFloat(Kokkos::Tools::Experimental::VariableValue value) {
  switch (value.metadata->type) {
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_int64:
    return static_cast<float>(value.value.int_value);
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_double:
    return static_cast<float>(value.value.double_value);
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_string:
    return static_cast<float>(
        std::hash<std::string>{}(value.value.string_value)); // terrible
  default:
    return 0.0; // TODO: break in some fashion instead
  }
}

Kokkos::Tools::Experimental::VariableValue
mvv(size_t index, Kokkos::Tools::Experimental::VariableValue reference,
    Kokkos::Tools::Experimental::ValueSet &set) {
  Kokkos::Tools::Experimental::VariableValue value;
  value.type_id = reference.type_id;
  value.metadata = reference.metadata;
  switch (reference.metadata->type) {
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_int64:
    value.value.int_value = set.values.int_value[index];
    break;
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_double:
    value.value.double_value = set.values.double_value[index];
    break;
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_string:
    strncpy(value.value.string_value, set.values.string_value[index],
            KOKKOS_TOOLS_TUNING_STRING_LENGTH);
    break;
  }
  return value;
}

std::string get_region_name(variableSet variables) {
  std::string name;
  for (int x = 0; x < variables.num_variables; ++x) {
    auto *database_data = reinterpret_cast<VariableDatabaseData *>(
        variables.variable_ids[x].metadata->toolProvidedInfo);
    std::string local_name(database_data->name);
    name += local_name;
    if (x == (variables.num_variables - 1)) {
      name += ".yaml";
    } else {
      name += "_";
    }
  }
  return name;
}
bool file_exists(std::string path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}
extern "C" void kokkosp_request_values(
    size_t contextId, size_t numContextVariables,
    Kokkos::Tools::Experimental::VariableValue *contextValues,
    size_t numTuningVariables,
    Kokkos::Tools::Experimental::VariableValue *tuningValues) {
  if (numTuningVariables == 0) {
    return;
  }
  static int created_regions;
  int64_t choiceSpaceSize = 1;
  for (int x = 0; x < numTuningVariables; ++x) {
    auto *metadata = reinterpret_cast<VariableDatabaseData *>(
        tuningValues[x].metadata->toolProvidedInfo);
    auto set_size = metadata->candidate_set_size;
    if (choiceSpaceSize > max_choices / set_size) {
      printf("Apollo Tuner: too many choices\n");
    }
    choiceSpaceSize *= set_size;
  }
  variableSet tuningProblem;
  int numValidVariables = 0;
  for (int x = 0; x < numContextVariables; ++x) {
    if (untunables.find(contextValues[x].type_id) == untunables.end()) {
      tuningProblem.variable_ids[numValidVariables++] = contextValues[x];
    }
  }
  for (int x = 0; x < numTuningVariables; ++x) {
    tuningProblem.variable_ids[numValidVariables++] = tuningValues[x];
  }
  tuningProblem.num_variables = numValidVariables;
  auto iter =
      tuning_regions.emplace(tuningProblem, std::make_pair("", nullptr));
  if (iter.second) {
    std::string prefix = "dtree-step-0-rank-0-";
    std::string suffix = ".yaml";
    std::string name = get_region_name(tuningProblem);
    constexpr const int apollo_user_file_length = 63;
    name = name.substr(0, apollo_user_file_length);
    std::string final_name = prefix + name + suffix;
    Apollo::Region *region;
    if (file_exists(final_name)) {
      region = new Apollo::Region(numContextVariables, name.c_str(),
                                  choiceSpaceSize, nullptr, final_name);
    } else {
      region = new Apollo::Region(numContextVariables, name.c_str(),
                                  choiceSpaceSize);
      ++num_unconverged_regions;
    }
    iter.first->second = std::make_pair(name, region);
  }
  auto region = iter.first->second.second;
  auto *context = region->begin();
  tuned_contexts[contextId] = std::make_pair(region, context);
  for (int x = 0; x < numContextVariables; ++x) {
    if (untunables.find(contextValues[x].type_id) == untunables.end()) {
      region->setFeature(context, variableToFloat(contextValues[x]));
    }
  }

  int policyChoice = region->getPolicyIndex(context);

  for (int x = 0; x < numTuningVariables; ++x) {
    auto *metadata = reinterpret_cast<VariableDatabaseData *>(
        tuningValues[x].metadata->toolProvidedInfo);
    int set_size = metadata->candidate_set_size;
    int local_policy_choice = policyChoice % set_size;
    tuningValues[x] = mvv(tuningValues[x].type_id,
                          metadata->candidate_values[local_policy_choice].value,
                          (tuningValues[x].metadata));
    policyChoice /= set_size;
  }
}

extern "C" void kokkosp_end_context(size_t contextId) {
  auto search = tuned_contexts.find(contextId);
  if (search == tuned_contexts.end()) {
    return;
  }
  auto region_context = search->second;
  auto region = region_context.first;
  auto context = region_context.second;
  region->end(context);
  tuned_contexts.erase(contextId);
  static int encounter;
  if ((++encounter % flush_interval) == 0) {
    apollo->flushAllRegionMeasurements(0);
    num_unconverged_regions = 0; // TODO: better
  }
}
