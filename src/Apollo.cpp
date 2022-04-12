// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/Apollo.h"

#include <execinfo.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "apollo/ModelFactory.h"
#include "apollo/Region.h"

#ifdef ENABLE_MPI
MPI_Comm apollo_mpi_comm;
#endif

namespace apolloUtils
{  //----------

inline std::string strToUpper(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return std::toupper(c);
  });
  return s;
}

inline void strReplaceAll(std::string &input,
                          const std::string &from,
                          const std::string &to)
{
  size_t pos = 0;
  while ((pos = input.find(from, pos)) != std::string::npos) {
    input.replace(pos, from.size(), to);
    pos += to.size();
  }
}

bool strOptionIsEnabled(std::string so)
{
  std::string sup = apolloUtils::strToUpper(so);
  if ((sup.compare("1") == 0) || (sup.compare("TRUE") == 0) ||
      (sup.compare("YES") == 0) || (sup.compare("ENABLED") == 0) ||
      (sup.compare("VERBOSE") == 0) || (sup.compare("ON") == 0)) {
    return true;
  } else {
    return false;
  }
}

inline const char *safeGetEnv(const char *var_name,
                              const char *use_this_if_not_found,
                              bool silent = false)
{
  char *c = getenv(var_name);
  if (c == NULL) {
    if (not silent) {
      std::cout << "== APOLLO: Looked for " << var_name
                << " with getenv(), found nothing, using '"
                << use_this_if_not_found << "' (default) instead." << std::endl;
    }
    return use_this_if_not_found;
  } else {
    return c;
  }
}


}  // namespace apolloUtils


std::string Apollo::getCallpathOffset(int walk_distance)
{
  // Using backtrace to walk the stack. Region id format is <module>@<addr>.
  void *buffer[walk_distance];
  char **stack_infos;

  backtrace(buffer, walk_distance);
  stack_infos = backtrace_symbols(buffer, walk_distance);

  std::string stack_info = stack_infos[walk_distance - 1];
  free(stack_infos);

  std::string module_name = stack_info.substr(0, stack_info.find_last_of(' '));
  module_name = module_name.substr(module_name.find_last_of("/\\") + 1);
  module_name = module_name.substr(0, module_name.find_last_of('('));
  std::string addr = stack_info.substr(stack_info.find(' '));
  unsigned start = addr.find_first_of('[') + 1;
  unsigned end = addr.find_last_of(']');
  addr = addr.substr(start, end - start);
  std::string region_id = module_name + "@" + addr;
  // std::cout << "region id " << region_id << "\n";
  return region_id;
}

Apollo::Apollo()
{
  region_executions = 0;

  // Initialize config with defaults
  Config::APOLLO_POLICY_MODEL =
      apolloUtils::safeGetEnv("APOLLO_POLICY_MODEL", "Static,policy=0");
  Config::APOLLO_COLLECTIVE_TRAINING =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_COLLECTIVE_TRAINING", "0"));
  Config::APOLLO_LOCAL_TRAINING =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_LOCAL_TRAINING", "1"));
  Config::APOLLO_SINGLE_MODEL =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_SINGLE_MODEL", "0"));
  Config::APOLLO_REGION_MODEL =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_REGION_MODEL", "1"));
  Config::APOLLO_GLOBAL_TRAIN_PERIOD =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_GLOBAL_TRAIN_PERIOD", "0"));
  Config::APOLLO_PER_REGION_TRAIN_PERIOD =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_PER_REGION_TRAIN_PERIOD", "0"));
  Config::APOLLO_TRACE_POLICY =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_TRACE_POLICY", "0"));
  Config::APOLLO_STORE_MODELS =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_STORE_MODELS", "0"));
  Config::APOLLO_TRACE_RETRAIN =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_TRACE_RETRAIN", "0"));
  Config::APOLLO_TRACE_ALLGATHER =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_TRACE_ALLGATHER", "0"));
  Config::APOLLO_TRACE_BEST_POLICIES =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_TRACE_BEST_POLICIES", "0"));
  Config::APOLLO_RETRAIN_ENABLE =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_RETRAIN_ENABLE", "0"));
  Config::APOLLO_RETRAIN_TIME_THRESHOLD = std::stof(
      apolloUtils::safeGetEnv("APOLLO_RETRAIN_TIME_THRESHOLD", "2.0"));
  Config::APOLLO_RETRAIN_REGION_THRESHOLD = std::stof(
      apolloUtils::safeGetEnv("APOLLO_RETRAIN_REGION_THRESHOLD", "0.5"));
  Config::APOLLO_TRACE_CSV =
      std::stoi(apolloUtils::safeGetEnv("APOLLO_TRACE_CSV", "0"));
  Config::APOLLO_TRACE_CSV_FOLDER_SUFFIX =
      apolloUtils::safeGetEnv("APOLLO_TRACE_CSV_FOLDER_SUFFIX", "");

  if (Config::APOLLO_COLLECTIVE_TRAINING) {
#ifndef ENABLE_MPI
    std::cerr << "Collective training requires MPI support to be enabled"
              << std::endl;
    abort();
#endif  // ENABLE_MPI
  }

  if (Config::APOLLO_COLLECTIVE_TRAINING && Config::APOLLO_LOCAL_TRAINING) {
    std::cerr << "Both collective and local training cannot be enabled"
              << std::endl;
    abort();
  }

  if (!(Config::APOLLO_COLLECTIVE_TRAINING || Config::APOLLO_LOCAL_TRAINING)) {
    std::cerr << "Either collective or local training must be enabled"
              << std::endl;
    abort();
  }

  if (Config::APOLLO_SINGLE_MODEL && Config::APOLLO_REGION_MODEL) {
    std::cerr << "Both global and region modeling cannot be enabled"
              << std::endl;
    abort();
  }


  if (!(Config::APOLLO_SINGLE_MODEL || Config::APOLLO_REGION_MODEL)) {
    std::cerr << "Either global or region modeling must be enabled"
              << std::endl;
    abort();
  }

#ifdef ENABLE_MPI
  MPI_Comm_dup(MPI_COMM_WORLD, &apollo_mpi_comm);
  MPI_Comm_rank(apollo_mpi_comm, &mpiRank);
  MPI_Comm_size(apollo_mpi_comm, &mpiSize);
#else
  mpiSize = 1;
  mpiRank = 0;
#endif  // ENABLE_MPI

  return;
}

Apollo::~Apollo()
{
  for (auto &it : regions) {
    Region *r = it.second;
    delete r;
  }
  std::cerr << "Apollo: total region executions: " << region_executions
            << std::endl;
}

#ifdef ENABLE_MPI
int get_mpi_pack_measure_size(int num_features, MPI_Comm comm)
{
  int size = 0, measure_size = 0;
  // rank
  MPI_Pack_size(1, MPI_INT, comm, &size);
  measure_size += size;
  // num features
  MPI_Pack_size(1, MPI_INT, comm, &size);
  measure_size += size;
  // feature vector
  MPI_Pack_size(num_features, MPI_FLOAT, comm, &size);
  measure_size += size;
  // policy
  MPI_Pack_size(1, MPI_INT, comm, &size);
  measure_size += size;
  // region name
  MPI_Pack_size(64, MPI_CHAR, comm, &size);
  measure_size += size;
  // metric
  MPI_Pack_size(1, MPI_DOUBLE, comm, &size);
  measure_size += size;

  return measure_size;
}

void packMeasurements(char *buf, int size, int mpiRank, Apollo::Region *reg)
{
  int pos = 0;

  auto measures = reg->dataset.toVectorOfTuples();
  for (auto &measure : measures) {
    const auto &features = std::get<0>(measure);
    const int &policy = std::get<1>(measure);
    const double &metric = std::get<2>(measure);

    // rank
    MPI_Pack(&mpiRank, 1, MPI_INT, buf, size, &pos, apollo_mpi_comm);
    // std::cout << "rank," << rank << " pos: " << pos << std::endl;

    // num features
    MPI_Pack(&reg->num_features, 1, MPI_INT, buf, size, &pos, apollo_mpi_comm);
    // std::cout << "rank," << rank << " pos: " << pos << std::endl;

    // feature vector
    for (const float &value : features) {
      MPI_Pack(&value, 1, MPI_FLOAT, buf, size, &pos, apollo_mpi_comm);
      // std::cout << "feature," << value << " pos: " << pos << std::endl;
    }

    // policy index
    MPI_Pack(&policy, 1, MPI_INT, buf, size, &pos, apollo_mpi_comm);
    // std::cout << "policy_index," << policy_index << " pos: " << pos <<
    // std::endl;
    //  XXX: use 64 bytes fixed for region_name
    //  region name
    MPI_Pack(reg->name, 64, MPI_CHAR, buf, size, &pos, apollo_mpi_comm);
    // std::cout << "region_name," << name << " pos: " << pos << std::endl;
    //  average time
    MPI_Pack(&metric, 1, MPI_DOUBLE, buf, size, &pos, apollo_mpi_comm);
    // std::cout << "time_avg," << time_avg << " pos: " << pos << std::endl;
  }
  return;
}
#endif


void Apollo::gatherCollectiveTrainingData(int step)
{
#ifdef ENABLE_MPI
  // MPI is enabled, proceed...
  int send_size = 0;
  for (auto &it : regions) {
    Region *reg = it.second;
    send_size +=
        (get_mpi_pack_measure_size(reg->num_features, apollo_mpi_comm) *
         reg->dataset.size());
  }

  char *sendbuf = (char *)malloc(send_size);
  // std::cout << "send_size: " << send_size << std::endl;
  int offset = 0;
  for (auto it = regions.begin(); it != regions.end(); ++it) {
    Region *reg = it->second;
    int reg_dataset_size =
        (reg->dataset.size() *
         get_mpi_pack_measure_size(reg->num_features, apollo_mpi_comm));
    packMeasurements(sendbuf + offset, reg_dataset_size, mpiRank, reg);
    offset += reg_dataset_size;
  }

  int num_ranks = mpiSize;
  // std::cout << "num_ranks: " << num_ranks << std::endl;

  int recv_size_per_rank[num_ranks];

  MPI_Allgather(
      &send_size, 1, MPI_INT, &recv_size_per_rank, 1, MPI_INT, apollo_mpi_comm);

  int recv_size = 0;
  // std::cout << "RECV SIZES: " ;
  for (int i = 0; i < num_ranks; i++) {
    // std::cout << i << ":" << recv_size_per_rank[i] << ", ";
    recv_size += recv_size_per_rank[i];
  }
  // std::cout << std::endl;

  char *recvbuf = (char *)malloc(recv_size);

  int disp[num_ranks];
  disp[0] = 0;
  for (int i = 1; i < num_ranks; i++) {
    disp[i] = disp[i - 1] + recv_size_per_rank[i - 1];
  }

  //std::cout <<"DISP: "; \
    for(int i = 0; i < num_ranks; i++) { \
        std::cout << i << ":" << disp[i] << ", "; \
    } \
    std::cout << std::endl;

  MPI_Allgatherv(sendbuf,
                 send_size,
                 MPI_PACKED,
                 recvbuf,
                 recv_size_per_rank,
                 disp,
                 MPI_PACKED,
                 apollo_mpi_comm);

  // std::cout << "BYTES TRANSFERRED: " << recv_size << std::endl;

  std::stringstream trace_out;
  if (Config::APOLLO_TRACE_ALLGATHER)
    trace_out << "rank, region_name, features, policy, time_avg" << std::endl;
  // std::cout << "Rank " << rank << " TOTAL_MEASURES: " << total_measures <<
  // std::endl;

  //int my_rank; \
    MPI_Comm_rank( comm, &my_rank ); \
    std::ofstream fout("step-" + std::to_string(step) + \
                "-rank-" + std::to_string(my_rank) + "-gather.txt");
  //std::stringstream dbgout; \
    dbgout << "rank, region_name, features, policy_index, time_avg" << std::endl;

  int pos = 0;
  while (pos < recv_size) {
    int rank;
    int num_features;
    std::vector<float> features;
    int policy;
    char region_name[64];
    int exec_count;
    double metric;

    MPI_Unpack(recvbuf, recv_size, &pos, &rank, 1, MPI_INT, apollo_mpi_comm);
    MPI_Unpack(
        recvbuf, recv_size, &pos, &num_features, 1, MPI_INT, apollo_mpi_comm);
    for (int j = 0; j < num_features; j++) {
      float value;
      MPI_Unpack(
          recvbuf, recv_size, &pos, &value, 1, MPI_FLOAT, apollo_mpi_comm);
      features.push_back(value);
    }
    MPI_Unpack(recvbuf, recv_size, &pos, &policy, 1, MPI_INT, apollo_mpi_comm);
    MPI_Unpack(
        recvbuf, recv_size, &pos, region_name, 64, MPI_CHAR, apollo_mpi_comm);
    MPI_Unpack(
        recvbuf, recv_size, &pos, &metric, 1, MPI_DOUBLE, apollo_mpi_comm);

    if (Config::APOLLO_TRACE_ALLGATHER) {
      trace_out << rank << ", " << region_name << ", ";
      trace_out << "[ ";
      for (auto &f : features) {
        trace_out << (int)f << ", ";
      }
      trace_out << "], ";
      trace_out << policy << ", " << metric << std::endl;
    }

    // Do not re-insert this rank's measurements
    if (rank == mpiRank) continue;

    // Find local region to reduce collective training data
    // TODO keep unseen regions to boostrap their models on execution?
    auto reg_iter = regions.find(region_name);
    if (reg_iter != regions.end()) {
      Region *reg = reg_iter->second;
      reg->dataset.insert(features, policy, metric);
    }
  }

  if (Config::APOLLO_TRACE_ALLGATHER) {
    std::cout << trace_out.str() << std::endl;
    std::ofstream fout("step-" + std::to_string(step) + "-rank-" +
                       std::to_string(mpiRank) + "-allgather.txt");

    fout << trace_out.str();
    fout.close();
  }

  free(sendbuf);
  free(recvbuf);
#else
  throw std::runtime_error("Expected MPI enabled");
#endif  // ENABLE_MPI
}

// DEPRECATED, use train.
void Apollo::flushAllRegionMeasurements(int step) { train(step); }

void Apollo::train(int step, bool doCollectPendingContexts)
{
  int rank = mpiRank;  // Automatically 0 if not an MPI environment.

  if (Config::APOLLO_COLLECTIVE_TRAINING) {
    // std::cout << "DO COLLECTIVE TRAINING" << std::endl; //ggout
    gatherCollectiveTrainingData(step);
  } else {
    // std::cout << "DO LOCAL TRAINING" << std::endl; //ggout
  }

  // Create a single model using all per-region measurements
  if (Config::APOLLO_SINGLE_MODEL) {
    Apollo::Dataset merged_dataset;
    for (auto &it : regions) {
      Region *reg = it.second;
      // append per-region dataset to merged.
      merged_dataset.insert(reg->dataset);
    }

    // TODO: Each region trains its own identical, single model. Consider
    // training the single model once and share to regions using a
    // shared_ptr.
    for (auto &it : regions) {
      Region *reg = it.second;
      // XXX: assumes all regions have the same model name, number of
      // features, number of policies, model_params
      reg->model->train(merged_dataset);
    }
  } else {
    for (auto &it : regions) {
      Region *reg = it.second;
      reg->train(step, doCollectPendingContexts);
    }
  }

  return;
}

extern "C" {
void *__apollo_region_create(int num_features,
                             const char *id,
                             int num_policies,
                             int min_training_data,
                             const char *model_info)
{
  static Apollo *apollo = Apollo::instance();
  // std::string callpathOffset = apollo->getCallpathOffset(3);
  std::cout << "CREATE region " << id << " num_features " << num_features
            << " num policies " << num_policies << std::endl;
  return new Apollo::Region(
      num_features, id, num_policies, min_training_data, model_info);
}

void __apollo_region_begin(Apollo::Region *r)
{
  // std::cout << "BEGIN region " << r->name << std::endl;
  r->begin();
}

void __apollo_region_end(Apollo::Region *r)
{
  // std::cout << "END region " << r->name << std::endl;
  r->end();
}

void __apollo_region_set_feature(Apollo::Region *r, float feature)
{
  // std::cout << "SET FEATURE " << feature << " region " << r->name <<
  // std::endl;
  r->setFeature(feature);
}

int __apollo_region_get_policy(Apollo::Region *r)
{
  int policy = r->getPolicyIndex();
  // std::cout << "GET POLICY " << policy << " region " << r->name << std::endl;
  return policy;
}

void __apollo_region_train(Apollo::Region *r, int step) { r->train(step); }
}
