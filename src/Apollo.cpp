
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

//
#include "util/Debug.h"

#ifdef ENABLE_MPI
    MPI_Comm apollo_mpi_comm;
#endif

//TODO(cdw): Move this into a private 'Utils' class within Apollo namespace.
namespace apolloUtils { //----------

inline std::string
strToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) {
            return std::toupper(c);
        });
    return s;
}

inline void
strReplaceAll(std::string& input, const std::string& from, const std::string& to) {
	size_t pos = 0;
	while ((pos = input.find(from, pos)) != std::string::npos) {
		input.replace(pos, from.size(), to);
		pos += to.size();
	}
}

bool
strOptionIsEnabled(std::string so) {
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

    region_executions = 0;

    // Initialize config with defaults
    Config::APOLLO_INIT_MODEL          = apolloUtils::safeGetEnv( "APOLLO_INIT_MODEL", "Static,0" );
    Config::APOLLO_COLLECTIVE_TRAINING = std::stoi( apolloUtils::safeGetEnv( "APOLLO_COLLECTIVE_TRAINING", "1" ) );
    Config::APOLLO_LOCAL_TRAINING      = std::stoi( apolloUtils::safeGetEnv( "APOLLO_LOCAL_TRAINING", "0" ) );
    Config::APOLLO_SINGLE_MODEL        = std::stoi( apolloUtils::safeGetEnv( "APOLLO_SINGLE_MODEL", "0" ) );
    Config::APOLLO_REGION_MODEL        = std::stoi( apolloUtils::safeGetEnv( "APOLLO_REGION_MODEL", "1" ) );
    Config::APOLLO_TRACE_MEASURES      = std::stoi( apolloUtils::safeGetEnv( "APOLLO_TRACE_MEASURES", "0" ) );
    Config::APOLLO_NUM_POLICIES        = std::stoi( apolloUtils::safeGetEnv( "APOLLO_NUM_POLICIES", "0" ) );
    Config::APOLLO_FLUSH_PERIOD       = std::stoi( apolloUtils::safeGetEnv( "APOLLO_FLUSH_PERIOD", "0" ) );
    Config::APOLLO_TRACE_POLICY        = std::stoi( apolloUtils::safeGetEnv( "APOLLO_TRACE_POLICY", "0" ) );
    Config::APOLLO_STORE_MODELS        = std::stoi( apolloUtils::safeGetEnv( "APOLLO_STORE_MODELS", "0" ) );
    Config::APOLLO_TRACE_RETRAIN       = std::stoi( apolloUtils::safeGetEnv( "APOLLO_TRACE_RETRAIN", "0" ) );
    Config::APOLLO_TRACE_ALLGATHER     = std::stoi( apolloUtils::safeGetEnv( "APOLLO_TRACE_ALLGATHER", "0" ) );
    Config::APOLLO_TRACE_BEST_POLICIES = std::stoi( apolloUtils::safeGetEnv( "APOLLO_TRACE_BEST_POLICIES", "0" ) );
    Config::APOLLO_RETRAIN_ENABLE      = std::stoi( apolloUtils::safeGetEnv( "APOLLO_RETRAIN_ENABLE", "1" ) );
    Config::APOLLO_RETRAIN_TIME_THRESHOLD   = std::stof( apolloUtils::safeGetEnv( "APOLLO_RETRAIN_TIME_THRESHOLD", "2.0" ) );
    Config::APOLLO_RETRAIN_REGION_THRESHOLD = std::stof( apolloUtils::safeGetEnv( "APOLLO_RETRAIN_REGION_THRESHOLD", "0.5" ) );

    //std::cout << "init model " << Config::APOLLO_INIT_MODEL << std::endl;
    //std::cout << "collective " << Config::APOLLO_COLLECTIVE_TRAINING << std::endl;
    //std::cout << "local "      << Config::APOLLO_LOCAL_TRAINING << std::endl;
    //std::cout << "global "     << Config::APOLLO_SINGLE_MODEL << std::endl;
    //std::cout << "region "     << Config::APOLLO_REGION_MODEL << std::endl;

#ifndef ENABLE_MPI
    // MPI is disabled...
    if ( Config::APOLLO_COLLECTIVE_TRAINING ) {
        std::cerr << "Collective training requires MPI support to be enabled" << std::endl;
        abort();
    }
    //TODO[chad]: Deepen this sanity check when additional collectives/training
    //            backends are added to the code.
#endif //ENABLE_MPI

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
            traceOutputFileName += ".";
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

#ifdef ENABLE_MPI
    if ( Config::APOLLO_COLLECTIVE_TRAINING ) {
        MPI_Comm_dup(MPI_COMM_WORLD, &apollo_mpi_comm);
    }
    // Initialize mpi rank and size even if doing local training with MPI.
    MPI_Comm_rank(apollo_mpi_comm, &mpiRank);
    MPI_Comm_size(apollo_mpi_comm, &mpiSize);
#else
    mpiSize = 1;
    mpiRank = 0;
#endif //ENABLE_MPI

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
// TODO(cdw): Generalize the fields slightly for CUDA also...
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

#ifdef ENABLE_MPI
int
get_mpi_pack_measure_size(int num_features, MPI_Comm comm)
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
packMeasurements(char *buf, int size, int mpiRank, Apollo::Region *reg) {
    int pos = 0;

    for( auto &it : reg->best_policies ) {
        auto &feature_vector = it.first;
        int policy_index = it.second.first;
        double time_avg = it.second.second;

        // rank
        MPI_Pack( &mpiRank, 1, MPI_INT, buf, size, &pos, apollo_mpi_comm );
        //std::cout << "rank," << rank << " pos: " << pos << std::endl;

        // num features
        MPI_Pack( &reg->num_features, 1, MPI_INT, buf, size, &pos, apollo_mpi_comm );
        //std::cout << "rank," << rank << " pos: " << pos << std::endl;

        // feature vector
        for (float value : feature_vector ) {
            MPI_Pack( &value, 1, MPI_FLOAT, buf, size, &pos, apollo_mpi_comm);
            //std::cout << "feature," << value << " pos: " << pos << std::endl;
        }

        // policy index
        MPI_Pack( &policy_index, 1, MPI_INT, buf, size, &pos, apollo_mpi_comm);
        //std::cout << "policy_index," << policy_index << " pos: " << pos << std::endl;
        // XXX: use 64 bytes fixed for region_name
        // region name
        MPI_Pack( reg->name, 64, MPI_CHAR, buf, size, &pos, apollo_mpi_comm);
        //std::cout << "region_name," << name << " pos: " << pos << std::endl;
        // average time
        MPI_Pack( &time_avg, 1, MPI_DOUBLE, buf, size, &pos, apollo_mpi_comm);
        //std::cout << "time_avg," << time_avg << " pos: " << pos << std::endl;
    }
    return;
}
#endif


void
Apollo::gatherReduceCollectiveTrainingData(int step)
{
#ifndef ENABLE_MPI
    // MPI is disabled, skip everything in this method.
    //
    // NOTE[chad]: Skipping this entire method is equivilant to
    //             doing LOCAL only training.  This ifdef guard is
    //             redundant to the one in flush, anticipating adding
    //             generic abstraction for collectives to support
    //             different backends or learning scenarios (SOS,
    //             python SKL modeling, etc.)
#else
    // MPI is enabled, proceed...
    int send_size = 0;
    for( auto &it: regions ) {
        Region *reg = it.second;
        // XXX assumes reg->reduceBestPolicies() has run
        send_size += ( get_mpi_pack_measure_size( reg->num_features, apollo_mpi_comm ) * reg->best_policies.size() );
    }

    char *sendbuf = (char *)malloc( send_size );
    //std::cout << "send_size: " << send_size << std::endl;
    int offset = 0;
    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Region *reg = it->second;
        int reg_measures_size = ( reg->best_policies.size() * get_mpi_pack_measure_size( reg->num_features, apollo_mpi_comm ) );
        packMeasurements( sendbuf + offset, reg_measures_size, mpiRank, reg );
        offset += reg_measures_size;
    }

    int num_ranks = mpiSize;
    //std::cout << "num_ranks: " << num_ranks << std::endl;

    int recv_size_per_rank[ num_ranks ];

    MPI_Allgather( &send_size, 1, MPI_INT, &recv_size_per_rank, 1, MPI_INT, apollo_mpi_comm);

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
            recvbuf, recv_size_per_rank, disp, MPI_PACKED, apollo_mpi_comm );

    //std::cout << "BYTES TRANSFERRED: " << num_measures * measure_size << std::endl;

    std::stringstream trace_out;
    if( Config::APOLLO_TRACE_ALLGATHER )
        trace_out << "rank, region_name, features, policy, time_avg" << std::endl;
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

        MPI_Unpack(recvbuf, recv_size, &pos, &rank, 1, MPI_INT, apollo_mpi_comm);
        MPI_Unpack(recvbuf, recv_size, &pos, &num_features, 1, MPI_INT, apollo_mpi_comm);
        for(int j = 0; j < num_features; j++)  {
            float value;
            MPI_Unpack(recvbuf, recv_size, &pos, &value, 1, MPI_FLOAT, apollo_mpi_comm);
            feature_vector.push_back( value );
        }
        MPI_Unpack(recvbuf, recv_size, &pos, &policy_index, 1, MPI_INT, apollo_mpi_comm);
        MPI_Unpack(recvbuf, recv_size, &pos, region_name, 64, MPI_CHAR, apollo_mpi_comm);
        MPI_Unpack(recvbuf, recv_size, &pos, &time_avg, 1, MPI_DOUBLE, apollo_mpi_comm);

        if( Config::APOLLO_TRACE_ALLGATHER ) {
            trace_out << rank << ", " << region_name << ", ";
            trace_out << "[ ";
            for(auto &f : feature_vector) {
                trace_out << (int)f << ", ";
            }
            trace_out << "], ";
            trace_out << policy_index << ", " << time_avg << std::endl;
        }

        // Find local region to reduce collective training data
        // TODO keep unseen regions to boostrap their models on execution?
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
    }

    if( Config::APOLLO_TRACE_ALLGATHER ) {
        std::cout << trace_out.str() << std::endl;
        std::ofstream fout("step-" + std::to_string(step) + \
                "-rank-" + std::to_string(mpiRank) + "-allgather.txt");

        fout << trace_out.str();
        fout.close();
    }

    free( sendbuf );
    free( recvbuf );
#endif //ENABLE_MPI
}


void
Apollo::flushAllRegionMeasurements(int step)
{
    int rank = mpiRank;  //Automatically 0 if not an MPI environment.

    // Reduce local region measurements to best policies
    // NOTE[chad]: reg->reduceBestPolicies() will guard any MPI collectives
    //             internally, and skip them if MPI is disabled.
    //             We may want to allow this method to do other non-MPI
    //             operations to regions, so we will call into it even if
    //             MPI is disabled. Ideally the flushAllRegionMeasurements
    //             method we are in now is only being called once per
    //             simulation step, so this should have negligible performance
    //             impact.
    for( auto &it: regions ) {
        Region *reg = it.second;
        reg->reduceBestPolicies(step);
        reg->measures.clear();
    }

    if( Config::APOLLO_COLLECTIVE_TRAINING ) {
        //std::cout << "DO COLLECTIVE TRAINING" << std::endl; //ggout
        gatherReduceCollectiveTrainingData(step);
    }
    else {
        //std::cout << "DO LOCAL TRAINING" << std::endl; //ggout
    }

    std::vector< std::vector<float> > train_features;
    std::vector< int > train_responses;

    std::vector< std::vector< float > > train_time_features;
    std::vector< float > train_time_responses;

    // Create a single model and fill the training vectors
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
        }

        //std::cout << "GLOBAL TRAINING " << std::endl;
        for(auto &it : best_policies_global) {
            train_features.push_back( it.first );
            train_responses.push_back( it.second.first );

            std::vector< float > feature_vector = it.first;
            feature_vector.push_back( it.second.first );
            train_time_features.push_back( feature_vector );
            train_time_responses.push_back( it.second.second );
        }

        best_policies_global.clear();
    }

    // Update the model to all regions
    for( auto &it : regions ) {
        Region *reg = it.second;

        if( reg->model->training && reg->best_policies.size() > 0 ) {
            if( Config::APOLLO_REGION_MODEL ) {
                //std::cout << "TRAIN MODEL PER REGION" << std::endl;
                // Reset training vectors
                train_features.clear();
                train_responses.clear();
                train_time_features.clear();
                train_time_responses.clear();

                // Prepare training data
                for(auto &it2 : reg->best_policies) {
                    train_features.push_back( it2.first );
                    train_responses.push_back( it2.second.first );

                    std::vector< float > feature_vector = it2.first;
                    feature_vector.push_back( it2.second.first );
                    train_time_features.push_back( feature_vector );
                    train_time_responses.push_back( it2.second.second );
                }
            }
            else {
                //std::cout << "ONE SINGLE MODEL" << std::endl;
            }

            if( Config::APOLLO_TRACE_BEST_POLICIES ) {
                std::stringstream trace_out;
                trace_out << "=== Rank " << rank \
                    << " BEST POLICIES Region " << reg->name << " ===" << std::endl;
                for( auto &b : reg->best_policies ) {
                    trace_out << "[ ";
                    for(auto &f : b.first)
                        trace_out << (int)f << ", ";
                    trace_out << "] P:" \
                        << b.second.first << " T: " << b.second.second << std::endl;
                }
                trace_out << ".-" << std::endl;
                std::cout << trace_out.str();
                std::ofstream fout("step-" + std::to_string(step) + \
                        "-rank-" + std::to_string(rank) + "-" + reg->name + "-best_policies.txt"); \
                    fout << trace_out.str(); \
                    fout.close();
            }

            // TODO(cdw): Load prior decisiontree...
            reg->model = ModelFactory::createDecisionTree(
                    num_policies,
                    train_features,
                    train_responses );

            reg->time_model = ModelFactory::createRegressionTree(
                    train_time_features,
                    train_time_responses );

            if( Config::APOLLO_STORE_MODELS ) {
                reg->model->store( "dtree-step-" + std::to_string( step ) \
                        + "-rank-" + std::to_string( rank ) \
                        + "-" + reg->name + ".yaml" );
                reg->time_model->store("regtree-step-" + std::to_string( step ) \
                        + "-rank-" + std::to_string( rank ) \
                        + "-" + reg->name + ".yaml");
            }
        }
        else {
            if( Config::APOLLO_RETRAIN_ENABLE && reg->time_model ) {
                //std::cout << "=== BEST POLICIES TRAINED REGION " << reg->name << " ===" << std::endl;
                //for( auto &b : reg->best_policies ) {
                //    std::cout << "[ " << (int)b.first[0] << " ]: P:" \
                //        << b.second.first << " T: " << b.second.second << std::endl;
                //}
                //std::cout << ".-" << std::endl;

                std::stringstream trace_out;
                // Check drifing regions for re-training
                int drifting = 0;
                for(auto &it2 : reg->best_policies) {
                    double time_avg = it2.second.second;

                    std::vector< float > feature_vector = it2.first;
                    feature_vector.push_back( it2.second.first );
                    double time_pred = reg->time_model->getTimePrediction( feature_vector );

                    if( time_avg > ( Config::APOLLO_RETRAIN_TIME_THRESHOLD * time_pred ) ) {
                        drifting++;
                        if( Config::APOLLO_TRACE_RETRAIN ) {
                            std::ios_base::fmtflags f( trace_out.flags() );
                            trace_out << std::setprecision(3) << std::scientific \
                                << "step " << step \
                                << " rank " << rank \
                                << " drift " << reg->name \
                                << "[ "; \
                                for(auto &f : it2.first ) { \
                                    trace_out << (int)f << ", "; \
                                } \
                            trace_out.flags( f ); \
                                trace_out << "] P " << it2.second.first \
                                << " time_avg " << time_avg \
                                << " time_pred " << time_pred \
                                << " time ratio " << ( time_avg / time_pred ) \
                                << std::endl;
                        }
                    }
                }

                if( drifting > 0 &&
                        ( static_cast<float>(drifting) / reg->best_policies.size() )
                        >=
                        Config::APOLLO_RETRAIN_REGION_THRESHOLD ) {
                    if( Config::APOLLO_TRACE_RETRAIN ) {
                        trace_out << "step " << step \
                            << " rank " << rank \
                            << " retrain " << reg->name \
                            << " drift ratio " \
                            << drifting << " / " << reg->best_policies.size() \
                            << std::endl;
                    }
                    //reg->model = ModelFactory::createRandom( num_policies );
                    reg->model = ModelFactory::createRoundRobin( num_policies );
                }

                if( Config::APOLLO_TRACE_RETRAIN ) {
                    std::cout << trace_out.str();
                    std::ofstream fout("step-" + std::to_string(step) \
                            + "-rank-" + std::to_string(rank) \
                            + "-retrain.txt");
                    fout << trace_out.str();
                    fout.close();
                }
            }
        }

        reg->best_policies.clear();
    }

    return;
}

extern "C" {
 void *__apollo_region_create(int num_features, char *id, int num_policies) {
     static Apollo *apollo = Apollo::instance();
     std::string callpathOffset = apollo->getCallpathOffset(3);
     std::cout << "CREATE region " << callpathOffset << " " << id << " num_features " << num_features
               << " num policies " << num_policies << std::endl;
     return new Apollo::Region(num_features, callpathOffset.c_str(), num_policies);
 }

 void __apollo_region_begin(Apollo::Region *r) {
     //std::cout << "BEGIN region " << r->name << std::endl;
     r->begin();
 }

 void __apollo_region_end(Apollo::Region *r) {
     //std::cout << "END region " << r->name << std::endl;
     r->end();
 }

 void __apollo_region_set_feature(Apollo::Region *r, float feature) {
     //std::cout << "SET FEATURE " << feature << " region " << r->name << std::endl;
     r->setFeature(feature);
 }

 int __apollo_region_get_policy(Apollo::Region *r) {
     int policy = r->getPolicyIndex();
     //std::cout << "GET POLICY " << policy << " region " << r->name << std::endl;
     return policy;
 }
}