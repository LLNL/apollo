
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


#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <algorithm>
#include <iomanip>

#include <omp.h>

#include "CallpathRuntime.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/Region.h"
#include "apollo/ModelFactory.h"

#define REGRESSION_TIME_THRESHOLD 1.5
#define RETRAIN_DRIFT_THRESHOLD 0.5

//
#include "util/Debug.h"

//TODO(cdw): Move this into a private 'Utils' class within Apollo namespace.
namespace apolloUtils { //----------

inline std::string strToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) {
            return std::toupper(c);
        });
    return s;
}

inline void strReplaceAll(std::string& input, const std::string& from, const std::string& to) {
	size_t pos = 0;
	while ((pos = input.find(from, pos)) != std::string::npos) {
		input.replace(pos, from.size(), to);
		pos += to.size();
	}
}

bool strOptionIsEnabled(std::string so) {
    std::string sup = apolloUtils::strToUpper(so);
    if ((sup.compare("1")           == 0) ||
        (sup.compare("TRUE")        == 0) ||
        (sup.compare("YES")         == 0) ||
        (sup.compare("ENABLED")     == 0) ||
        (sup.compare("VERBOSE")     == 0) ||
        (sup.compare("ON")          == 0)) {
        return true;
    } else {
        return false;
    }
}

inline const char*
safeGetEnv(
        const char *var_name,
        const char *use_this_if_not_found,
        bool        silent=false)
{
    char *c = getenv(var_name);
    if (c == NULL) {
        if (not silent) {
            std::cout << "== APOLLO: Looked for " << var_name << " with getenv(), found nothing, using '" \
                << use_this_if_not_found << "' (default) instead." << std::endl;
    }
        return use_this_if_not_found;
    } else {
        return c;
    }
}


} //end: namespace apolloUtils ----------


std::string
Apollo::getCallpathOffset(int walk_distance)
{
    //NOTE(cdw): Param 'walk_distance' is optional, defaults to 2
    //           so we walk out of this method, and then walk out
    //           of the wrapper code (i.e. a RAJA policy, or something
    //           performance tool instrumentation), and get the module
    //           and offset of the application's instantiation of
    //           a loop or steerable region.
    CallpathRuntime *cp = (CallpathRuntime *) callpath_ptr;
    // Set up this Region for the first time:       (Runs only once)
    std::stringstream ss_location;
    ss_location << cp->doStackwalk().get(walk_distance);
    // Extract out the pointer to our module+offset string and clean it up:
    std::string offsetstr = ss_location.str();
    offsetstr = offsetstr.substr((offsetstr.rfind("/") + 1), (offsetstr.length() - 1));
    apolloUtils::strReplaceAll(offsetstr, "(", "_");
    apolloUtils::strReplaceAll(offsetstr, ")", "_");

    return offsetstr;
}

Apollo::Apollo()
{
    callpath_ptr = new CallpathRuntime;

    // For other components of Apollo to access the SOS API w/out the include
    // file spreading SOS as a project build dependency, store these as void *
    // references in the class:
    log("Reading SLURM env...");
    numNodes         = std::stoi(apolloUtils::safeGetEnv("SLURM_NNODES", "1"));
    log("    numNodes ................: ", numNodes);
    numProcs         = std::stoi(apolloUtils::safeGetEnv("SLURM_NPROCS", "1"));
    log("    numProcs ................: ", numProcs);
    numCPUsOnNode    = std::stoi(apolloUtils::safeGetEnv("SLURM_CPUS_ON_NODE", "36"));
    log("    numCPUsOnNode ...........: ", numCPUsOnNode);
    std::string envProcPerNode = apolloUtils::safeGetEnv("SLURM_TASKS_PER_NODE", "1");
    // Sometimes SLURM sets this to something like "4(x2)" and
    // all we care about here is the "4":
    auto pos = envProcPerNode.find('(');
    if (pos != envProcPerNode.npos) {
        numProcsPerNode = std::stoi(envProcPerNode.substr(0, pos));
    } else {
        numProcsPerNode = std::stoi(envProcPerNode);
    }
    log("    numProcsPerNode .........: ", numProcsPerNode);

    numThreadsPerProcCap = std::max(1, (int)(numCPUsOnNode / numProcsPerNode));
    log("    numThreadsPerProcCap ....: ", numThreadsPerProcCap);

    log("Reading OMP env...");
    ompDefaultSchedule   = omp_sched_static;       //<-- libgomp.so default
    ompDefaultNumThreads = numThreadsPerProcCap;   //<-- from SLURM calc above
    ompDefaultChunkSize  = -1;                     //<-- let OMP decide

    // Override the OMP defaults if there are environment variables set:
    char *val = NULL;
    val = getenv("OMP_NUM_THREADS");
    if (val != NULL) {
        ompDefaultNumThreads = std::stoi(val);
    }

    val = NULL;
    // We assume nonmonotinicity and chunk size of -1 for now.
    val = getenv("OMP_SCHEDULE");
    if (val != NULL) {
        std::string sched = getenv("OMP_SCHEDULE");
        if ((sched.find("static") != sched.npos)
            || (sched.find("STATIC") != sched.npos)) {
            ompDefaultSchedule = omp_sched_static;
        } else if ((sched.find("dynamic") != sched.npos)
            || (sched.find("DYNAMIC") != sched.npos)) {
            ompDefaultSchedule = omp_sched_dynamic;
        } else if ((sched.find("guided") != sched.npos)
            || (sched.find("GUIDED") != sched.npos)) {
            ompDefaultSchedule = omp_sched_guided;
        }
    }

    numThreads = ompDefaultNumThreads;
    // Explicitly set the current OMP defaults, so LLVM's OMP library doesn't
    // run really slow for no reason:
    //NOTE(chad): Deprecated.
    //omp_set_num_threads(ompDefaultNumThreads);
    //omp_set_schedule(ompDefaultSchedule, -1);

    // Initialize config with defaults
    Config::APOLLO_INIT_MODEL = apolloUtils::safeGetEnv( "APOLLO_INIT_MODEL", "Static,0" );
    //std::cout << "init model " << Config::APOLLO_INIT_MODEL << std::endl;
    Config::APOLLO_COLLECTIVE_TRAINING = std::stoi( apolloUtils::safeGetEnv( "APOLLO_COLLECTIVE_TRAINING", "1" ) );
    //std::cout << "collective " << Config::APOLLO_COLLECTIVE_TRAINING << std::endl;
    Config::APOLLO_LOCAL_TRAINING = std::stoi( apolloUtils::safeGetEnv( "APOLLO_LOCAL_TRAINING", "0" ) );
    //std::cout << "local " << Config::APOLLO_LOCAL_TRAINING << std::endl;
    Config::APOLLO_SINGLE_MODEL = std::stoi( apolloUtils::safeGetEnv( "APOLLO_SINGLE_MODEL", "0" ) );
    //std::cout << "global " << Config::APOLLO_SINGLE_MODEL << std::endl;
    Config::APOLLO_REGION_MODEL = std::stoi( apolloUtils::safeGetEnv( "APOLLO_REGION_MODEL", "1" ) );
    //std::cout << "region " << Config::APOLLO_REGION_MODEL << std::endl;

    if( Config::APOLLO_COLLECTIVE_TRAINING && Config::APOLLO_LOCAL_TRAINING ) {
        std::cerr << "Both collective and local training cannot be enabled" << std::endl;
        abort();
    }

    if( ! ( Config::APOLLO_COLLECTIVE_TRAINING || Config::APOLLO_LOCAL_TRAINING ) ) {
        std::cerr << "Either collective or local training must be enabled" << std::endl;
        abort();
    }

    if( Config::APOLLO_SINGLE_MODEL && Config::APOLLO_REGION_MODEL ) {
        std::cerr << "Both global and region modeling cannot be enabled" << std::endl;
        abort();
    }


    if( ! ( Config::APOLLO_SINGLE_MODEL || Config::APOLLO_REGION_MODEL ) ) {
        std::cerr << "Either global or region modeling must be enabled" << std::endl;
        abort();
    }

    // Duplicate world for Apollo library communication
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_size(comm, &mpiSize);
    MPI_Comm_rank(comm, &mpiRank);

    //////////
    //
    // TODO(cdw): Move these implementation details into a dedicated 'Trace' class.
    //
    // Trace output settings:
    //
    std::string trace_enabled \
        = apolloUtils::safeGetEnv("APOLLO_TRACE_ENABLED", "false", true);
    std::string trace_emit_online \
        = apolloUtils::safeGetEnv("APOLLO_TRACE_EMIT_ONLINE", "false", true);
    std::string trace_emit_all_features \
        = apolloUtils::safeGetEnv("APOLLO_TRACE_EMIT_ALL_FEATURES", "false", true);
    std::string trace_output_file \
        = apolloUtils::safeGetEnv("APOLLO_TRACE_OUTPUT_FILE", "stdout", true);
    //
    traceEnabled          = ::apolloUtils::strOptionIsEnabled(trace_enabled);
    traceEmitOnline       = ::apolloUtils::strOptionIsEnabled(trace_emit_online);
    traceEmitAllFeatures  = ::apolloUtils::strOptionIsEnabled(trace_emit_all_features);
    //
    traceOutputFileName   = trace_output_file;
    if (traceEnabled) {
        if (traceOutputFileName.compare("stdout") == 0) {
            traceOutputIsActualFile = false;
        } else {
            traceOutputFileName += std::to_string(mpiRank);
            traceOutputFileName += ".csv";
            try {
                traceOutputFileHandle.open(traceOutputFileName, std::fstream::out);
                traceOutputIsActualFile = true;
            } catch (...) {
                std::cerr << "== APOLLO: ** ERROR ** Unable to open the filename specified in" \
                    << "APOLLO_TRACE_OUTPUT_FILE environment variable:" << std::endl;
                std::cerr << "== APOLLO: ** ERROR **    \"" << traceOutputFileName << "\"" << std::endl;
                std::cerr << "== APOLLO: ** ERROR ** Defaulting to std::cout ..." << std::endl;
                traceOutputIsActualFile = false;
            }
            if (traceEmitOnline) {
                writeTraceHeader();
            }
        }
    }
    //
    //////////

    log("Initialized.");

    return;
}

Apollo::~Apollo()
{
    // TODO(cdw): Move this into dedicated 'Trace' class in refactor.
    if (traceEnabled and (not traceEmitOnline)) {
        // We've been storing up our trace data to emit now.
        writeTraceHeader();
        writeTraceVector();
        if (traceOutputIsActualFile) {
            traceOutputFileHandle.close();
        }
    }

    delete (CallpathRuntime *)callpath_ptr;
}


//////////
//
// TODO(cdw): Move this into 'Trace' class during code refactor.
//
void
Apollo::writeTraceHeader(void) {
    if (not traceEnabled) { return; }

    if (traceOutputIsActualFile) {
        writeTraceHeaderImpl(traceOutputFileHandle);
    } else {
        writeTraceHeaderImpl(std::cout);
    }
}
//
void
Apollo::writeTraceHeaderImpl(std::ostream &sink) {
    if (not traceEnabled) { return; }

    std::string optional_column_label;
    if (traceEmitAllFeatures) {
        optional_column_label = ",all_features_json\n";
    } else {
        optional_column_label = "\n";
    }
    sink.precision(11);
    sink \
        << "type," \
        << "time_wall," \
        << "node_id," \
        << "step," \
        << "region_name," \
        << "policy_index," \
        << "num_threads," \
        << "num_elements," \
        << "time_exec" \
        << optional_column_label;
}
//
void Apollo::storeTraceLine(TraceLine_t &t) {
    if (not traceEnabled) { return; }

    trace_data.push_back(t);
}
//
void
Apollo::writeTraceLine(TraceLine_t &t) {
    if (not traceEnabled) { return; }
    if (traceOutputIsActualFile) {
        writeTraceLineImpl(t, traceOutputFileHandle);
    } else {
        writeTraceLineImpl(t, std::cout);
    }
}
//
void
Apollo::writeTraceLineImpl(TraceLine_t &t, std::ostream &sink) {
    if (not traceEnabled) { return; }
    sink \
       << "TRACE," << std::fixed \
       << std::get<0>(t) /* exec_time_end */ << "," \
       << std::get<1>(t) /* node_id */       << "," \
       << std::get<2>(t) /* comm_rank */     << "," \
       << std::get<3>(t) /* region_name */   << "," \
       << std::get<4>(t) /* policy_index */  << "," \
       << std::get<5>(t) /* num_threads */   << "," \
       << std::get<6>(t) /* num_elements */  << "," \
       << std::fixed << std::get<7>(t) /* (exec_time_end - exec_time_begin) */ \
       << std::get<8>(t) /* (", " + all features, optionally) */ \
       << "\n";
}
//
void
Apollo::writeTraceVector(void) {
    if (not traceEnabled) { return; }
    for (auto &t : trace_data) {
        writeTraceLine(t);
    }
}
//
//////////

int
get_measure_size(int num_features, MPI_Comm comm)
{
    int size = 0, measure_size = 0;
    // rank
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    // num features
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    // feature vector
    MPI_Pack_size( num_features, MPI_FLOAT, comm, &size);
    measure_size += size;
    // policy_index
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    // region name
    MPI_Pack_size( 64, MPI_CHAR, comm, &size);
    measure_size += size;
    // average time
    MPI_Pack_size( 1, MPI_DOUBLE, comm, &size);
    measure_size += size;

    return measure_size;
}

void
Apollo::gatherReduceCollectiveTrainingData(int step)
{
    int send_size = 0;
    for( auto &it: regions ) {
        Region *reg = it.second;
        // XXX assumes reg->reduceBestPolicies() has run
        send_size += ( get_measure_size( reg->num_features, comm ) * reg->best_policies.size() );
    }

    char *sendbuf = (char *)malloc( send_size );
    //std::cout << "send_size: " << send_size << std::endl;
    int offset = 0;
    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Region *reg = it->second;
        int reg_measures_size = ( reg->best_policies.size() * get_measure_size( reg->num_features, comm ) );
        reg->packMeasurements( sendbuf + offset, reg_measures_size, comm );
        offset += reg_measures_size;
    }

    int num_ranks;
    MPI_Comm_size( comm, &num_ranks );
    //std::cout << "num_ranks: " << num_ranks << std::endl;

    int recv_size_per_rank[ num_ranks ];

    MPI_Allgather( &send_size, 1, MPI_INT, &recv_size_per_rank, 1, MPI_INT, comm);

    int recv_size = 0;
    //std::cout << "RECV SIZES: " ;
    for(int i = 0; i < num_ranks; i++) {
      //std::cout << i << ":" << recv_size_per_rank[i] << ", ";
      recv_size += recv_size_per_rank[i];
    }
    //std::cout << std::endl;

    char *recvbuf = (char *) malloc( recv_size );

    int disp[ num_ranks ];
    disp[0] = 0;
    for(int i = 1; i < num_ranks; i++) {
        disp[i] = disp[i-1] + recv_size_per_rank[i-1];
    }

    //std::cout <<"DISP: "; \
    for(int i = 0; i < num_ranks; i++) { \
        std::cout << i << ":" << disp[i] << ", "; \
    } \
    std::cout << std::endl;

    MPI_Allgatherv( sendbuf, send_size, MPI_PACKED, \
            recvbuf, recv_size_per_rank, disp, MPI_PACKED, comm );

    //std::cout << "BYTES TRANSFERRED: " << num_measures * measure_size << std::endl;

    //std::cout << "Rank " << rank << " TOTAL_MEASURES: " << total_measures << std::endl;

    //int my_rank; \
    MPI_Comm_rank( comm, &my_rank ); \
    std::ofstream fout("step-" + std::to_string(step) + \
                "-rank-" + std::to_string(my_rank) + "-gather.txt");
    //std::stringstream dbgout; \
    dbgout << "rank, region_name, features, policy_index, time_avg" << std::endl;

    int pos = 0;
    while( pos < recv_size ) {
        int rank;
        int num_features;
        std::vector<float> feature_vector;
        int policy_index;
        char region_name[64];
        int exec_count;
        double time_avg;

        MPI_Unpack(recvbuf, recv_size, &pos, &rank, 1, MPI_INT, comm);
        MPI_Unpack(recvbuf, recv_size, &pos, &num_features, 1, MPI_INT, comm);
        for(int j = 0; j < num_features; j++)  {
            float value;
            MPI_Unpack(recvbuf, recv_size, &pos, &value, 1, MPI_FLOAT, comm);
            feature_vector.push_back( value );
        }
        MPI_Unpack(recvbuf, recv_size, &pos, &policy_index, 1, MPI_INT, comm);
        MPI_Unpack(recvbuf, recv_size, &pos, region_name, 64, MPI_CHAR, comm);
        MPI_Unpack(recvbuf, recv_size, &pos, &time_avg, 1, MPI_DOUBLE, comm);

        //dbgout << rank << ", " << region_name << ", "; \
            dbgout << "[ "; \
            for(auto &f : feature_vector) { \
                dbgout << (int)f << ", "; \
            } \
        dbgout << "], "; \
        dbgout << policy_index << ", " << time_avg << std::endl;

        // Find local region to reduce collective training data
        // TODO: keep unseen regions to boostrap their models on execution?
        auto reg_iter = regions.find( region_name );
        if( reg_iter != regions.end() ) {
            Region *reg = reg_iter->second;
            auto iter =  reg->best_policies.find( feature_vector );
            if( iter ==  reg->best_policies.end() ) {
                reg->best_policies.insert( { feature_vector, { policy_index, time_avg } } );
            }
            else {
                // Key exists
                if( reg->best_policies[ feature_vector ].second > time_avg ) {
                    reg->best_policies[ feature_vector ] = { policy_index, time_avg };
                }
            }
        }

        //if( reg_iter != regions.end() ) {
        //    Region *reg = reg_iter->second;
        //    std::cout << "=== BEST POLICIES COLLECTIVE " << region_name << " ===" << std::endl;
        //    for( auto &b : reg->best_policies) {
        //        std::cout << "[ " << (int)b.first[0] << " ]: P:" \
        //            << b.second.first << " T: " << b.second.second << " | ";
        //    }
        //    std::cout << "._" << std::endl;
        //    std::cout << "~~~~~~~~~" << std::endl;
        //}
    }

    //std::cout << dbgout.str() << std::endl;
    //fout << dbgout.str();
    //fout.close();

    free( sendbuf );
    free( recvbuf );
}

void
Apollo::flushAllRegionMeasurements(int step)
{
    // TODO: when to train?
    //if( step <= 1 ) {
    //    std::cout << "Skip " << step << std::endl;
    //    return;
    //}

    // Reduce local region measurements to best policies
    for( auto &it: regions ) {
        Region *reg = it.second;
        reg->reduceBestPolicies(step);
        // TODO: forget about old measures or not?
        reg->measures.clear();
    }

    if( Config::APOLLO_COLLECTIVE_TRAINING ) {
        //std::cout << "DO COLLECTIVE TRAINING" << std::endl; //ggout
        gatherReduceCollectiveTrainingData(step);
    }
    else {
        //std::cout << "DO LOCAL TRAINING" << std::endl; //ggout
    }

    if( Config::APOLLO_SINGLE_MODEL ) {
        // Reduce best polices per region to global
        for( auto &it: regions ) {
            Region *reg = it.second;
            for( auto &b : reg->best_policies ) {
                std::vector< float > feature_vector = b.first;
                int policy_index = b.second.first;
                double time_avg = b.second.second;

                auto iter = best_policies_global.find( feature_vector );
                if( iter == best_policies_global.end() ) {
                    best_policies_global.insert( { feature_vector, { policy_index, time_avg } } );
                }
                else {
                    // Key exists
                    if( best_policies_global[ feature_vector ].second > time_avg ) {
                        best_policies_global[ feature_vector ] = { policy_index, time_avg };
                    }
                }
            }

            //std::cout << "=== BEST POLICIES SINGLE " << reg->name << " ===" << std::endl;
            //for( auto &b : best_policies_global ) {
            //    std::cout << "[ " << (int)b.first[0] << " ]: P:" \
            //        << b.second.first << " T: " << b.second.second << " | ";
            //}
            //std::cout << "._" << std::endl;
        }

        std::vector< std::vector<float> > train_features;
        std::vector< int > train_responses;

        std::vector< std::vector< float > > train_time_features;
        std::vector< float > train_time_responses;

        //std::cout << "GLOBAL TRAINING " << std::endl;
        for(auto &it : best_policies_global) {
            train_features.push_back( it.first );
            train_responses.push_back( it.second.first );

            std::vector< float > feature_vector = it.first;
            feature_vector.push_back( it.second.first );
            train_time_features.push_back( feature_vector );
            train_time_responses.push_back( it.second.second );
        }
        //std::cout << ".-" << std::endl;

        //std::cout << "ONE GLOBAL TREE" << std::endl; //ggout
        // bool stored = false;
        // Update the model to all regions
        for(auto &it : regions) {
            Region *reg = it.second;
            if( reg->model->training && reg->best_policies.size() > 0 ) {
                // TODO use a shared_ptr and create once?
                reg->model = ModelFactory::createDecisionTree(
                        num_policies,
                        train_features,
                        train_responses );

                reg->time_model = ModelFactory::createRegressionTree(
                        train_time_features,
                        train_time_responses);

                //if( !stored ) {
                //reg->model->store( "dtree-" + std::to_string( step ) + ".yaml" );
                //reg->time_model->store("regtree-" + std::string(reg->name) + ".yaml");
                // stored = true;
                //
                //}
            }
            else {
                //TODO: re-train by regression?
                if( reg->time_model ) {
                    // Check for re-training
                    int drifting = 0;
                    for(auto &it2 : reg->best_policies) {
                        double time_avg = it2.second.second;

                        std::vector< float > feature_vector = it2.first;
                        feature_vector.push_back( it2.second.first );
                        double time_pred = reg->time_model->getTimePrediction( feature_vector );
                        // TODO: trigger re-train?
                        if( time_avg > REGRESSION_TIME_THRESHOLD*time_pred ) {
                            drifting++;
                            // ggout
                            //std::cout << "trigger retrain: " << reg->name \
                            << "[ " << feature_vector[0] << ", " << feature_vector[1] << " ]: " \
                                << " time_avg " << time_avg << " ~~ " \
                                << " time_pred " << time_pred << std::endl;
                        }
                    }

                    if( drifting > 0 &&
                            ( static_cast<float>(drifting) / reg->best_policies.size() ) > RETRAIN_DRIFT_THRESHOLD ) {
                        //std::cout << "retrain " << reg->name << " : " \
                        << drifting << " / " \
                            << reg->best_policies.size() << " total" << std::endl; //ggout
                        //reg->model = ModelFactory::createRandom( num_policies );
                        reg->model = ModelFactory::createRoundRobin( num_policies );
                    }

                    reg->best_policies.clear();
                }
            }
        }

        // TODO: forget or not training best_policies?
        best_policies_global.clear();
    }
    else {
        //std::cout << "TRAIN PER REGION MODEL" << std::endl;
        for(auto &it : regions ) {
            Region *reg = it.second;

            // TODO: if the region is training and there are measurements
            if( reg->model->training && reg->best_policies.size() > 0 ) {
                std::vector< std::vector<float> > train_features;
                std::vector< int > train_responses;

                std::vector< std::vector< float > > train_time_features;
                std::vector< float > train_time_responses;

                // Prepare training data
                for(auto &it2 : reg->best_policies) {
                    train_features.push_back( it2.first );
                    train_responses.push_back( it2.second.first );

                    std::vector< float > feature_vector = it2.first;
                    feature_vector.push_back( it2.second.first );
                    train_time_features.push_back( feature_vector );
                    train_time_responses.push_back( it2.second.second );
                }

                //std::stringstream dbgout;
                //int rank;
                //MPI_Comm_rank(comm, &rank);
                //dbgout << "=== Rank " << rank \
                //    << " BEST POLICIES Region " << reg->name << " ===" << std::endl;
                //for( auto &b : reg->best_policies ) {
                //    dbgout << "[ ";
                //    for(auto &f : b.first)
                //        dbgout << (int)f << ", ";
                //    dbgout << "]: P:" \
                //        << b.second.first << " T: " << b.second.second << std::endl;
                //}
                //dbgout << ".-" << std::endl;
                ////std::cout << dbgout.str();
                //std::ofstream fout("step-" + std::to_string(step) + \
                //        "-rank-" + std::to_string(rank) + "-" + reg->name + "-best_policies.txt"); \
                //fout << dbgout.str(); \
                //fout.close();

                //std::cout << "TRAIN TREE region " << reg->name << std::endl; //ggout
                reg->model = ModelFactory::createDecisionTree(
                        num_policies,
                        train_features,
                        train_responses );

                reg->time_model = ModelFactory::createRegressionTree(
                        train_time_features,
                        train_time_responses);
                //reg->time_model->store("regtree-" + std::string(reg->name) + ".yaml"); //ggout
                //reg->model->store( "dtree-" + std::to_string( step ) + "-" + reg->name + ".yaml" );
                // TODO: forget about already best_policies?
                reg->best_policies.clear();
            }
            else {
                if( reg->time_model ) {
                    //std::cout << "=== BEST POLICIES TRAINED REGION " << reg->name << " ===" << std::endl;
                    //for( auto &b : reg->best_policies ) {
                    //    std::cout << "[ " << (int)b.first[0] << " ]: P:" \
                    //        << b.second.first << " T: " << b.second.second << std::endl;
                    //}
                    //std::cout << ".-" << std::endl;

                    // Check for re-training
                    int drifting = 0;
                    for(auto &it2 : reg->best_policies) {
                        double time_avg = it2.second.second;

                        std::vector< float > feature_vector = it2.first;
                        feature_vector.push_back( it2.second.first );
                        double time_pred = reg->time_model->getTimePrediction( feature_vector );

                        //std::cout << "trigger retrain: " << reg->name \
                        //    << "[ " << feature_vector[0] << ", " << feature_vector[1] << " ]: " \
                        //    << " time_avg " << time_avg << " ~~ " \
                        //    << " time_pred " << time_pred \
                        //    << " rel_err " << ( time_avg - time_pred ) / time_pred << std::endl;

                        // TODO: trigger re-train?
                        if( time_avg > ( REGRESSION_TIME_THRESHOLD*time_pred ) ) {
                            drifting++;
                            // ggout
                            //std::ios_base::fmtflags f( std::cout.flags() );
                            //int rank;
                            //MPI_Comm_rank(comm, &rank);
                            //if( rank == 0) {
                            //    std::cout << std::setprecision(3) << std::scientific \
                            //        << "step " << step \
                            //        << " rank " << rank \
                            //        << " drift: " << reg->name \
                            //        << "[ "; \
                            //        for(auto &f : it2.first ) \
                            //            std::cout << (int)f << ", "; \
                            //    std::cout << "] @ " << it2.second.first \
                            //        << " time_avg " << time_avg << " ~~ " \
                            //        << " time_pred " << time_pred \
                            //        << " rel_err " << ( time_avg - time_pred ) / time_pred << std::endl;
                            //}
                            //std::cout.flags( f );
                        }
                    }

                    if( drifting > 0
                            && ( static_cast<float>(drifting) / reg->best_policies.size() ) > RETRAIN_DRIFT_THRESHOLD ) {
                        //std::cout << "retrain " << reg->name << " : " \
                        << drifting << " / " \
                            << reg->best_policies.size() << " total" << std::endl; //ggout
                        //reg->model = ModelFactory::createRandom( num_policies );
                        reg->model = ModelFactory::createRoundRobin( num_policies );
                        break;
                    }

                    reg->best_policies.clear();
                }
            }
        }

    }

    //int rank;
    //MPI_Comm_rank(comm, &rank);
    //std::cout << "=== FLUSH rank: " << rank \
    //    << " step: " << step \
    //    << " best_policies size: " << best_policies.size() << " ===" << std::endl; //ggout

    return;
}
