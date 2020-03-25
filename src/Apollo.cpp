
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

#include "apollo/models/RoundRobin.h"
#include "apollo/models/DecisionTree.h"
//
#include "util/Debug.h"

inline void replace_all(std::string& input, const std::string& from, const std::string& to) {
	size_t pos = 0;
	while ((pos = input.find(from, pos)) != std::string::npos) {
		input.replace(pos, from.size(), to);
		pos += to.size();
	}
}



std::string
Apollo::getCallpathOffset(int walk_distance)
{
    //NOTE(chad): Param 'walk_distance' is optional, defaults to 2
    //            so we walk out of this method, and then walk out
    //            of the wrapper code (i.e. a RAJA policy, or something
    //            performance tool instrumentation), and get the module
    //            and offset of the application's instantiation of
    //            a loop or steerable region.
    CallpathRuntime *cp = (CallpathRuntime *) callpath_ptr;
    // Set up this Region for the first time:       (Runs only once)
    std::stringstream ss_location;
    ss_location << cp->doStackwalk().get(walk_distance);
    // Extract out the pointer to our module+offset string and clean it up:
    std::string offsetstr = ss_location.str();
    offsetstr = offsetstr.substr((offsetstr.rfind("/") + 1), (offsetstr.length() - 1));
    replace_all(offsetstr, "(", "_");
    replace_all(offsetstr, ")", "_");

    return offsetstr;
}

Apollo::Apollo()
{
    callpath_ptr = new CallpathRuntime;

    // For other components of Apollo to access the SOS API w/out the include
    // file spreading SOS as a project build dependency, store these as void *
    // references in the class:
    log("Reading SLURM env...");
    numNodes         = std::stoi(safe_getenv("SLURM_NNODES", "1"));
    log("    numNodes ................: ", numNodes);
    numProcs         = std::stoi(safe_getenv("SLURM_NPROCS", "1"));
    log("    numProcs ................: ", numProcs);
    numCPUsOnNode    = std::stoi(safe_getenv("SLURM_CPUS_ON_NODE", "36"));
    log("    numCPUsOnNode ...........: ", numCPUsOnNode);
    std::string envProcPerNode = safe_getenv("SLURM_TASKS_PER_NODE", "1");
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

    // Duplicate world for Apollo library communication
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    log("Initialized.");

    return;
}

Apollo::~Apollo()
{
    delete callpath_ptr;
}

int 
get_measure_size(int num_features, MPI_Comm comm)
{
    int size = 0, measure_size = 0;
    // rank
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    // features
    MPI_Pack_size( num_features, MPI_FLOAT, comm, &size);
    measure_size += size;
    // policy_index
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    // region name
    MPI_Pack_size( 64, MPI_CHAR, comm, &size);
    measure_size += size;
    // execution count
    //MPI_Pack_size( 1, MPI_INT, comm, &size);
    //measure_size += size;
    // average time
    MPI_Pack_size( 1, MPI_DOUBLE, comm, &size);
    measure_size += size;

    return measure_size;
}

void
Apollo::flushAllRegionMeasurements(int assign_to_step)
{
    // TODO: when to train?
    // if( ( assign_to_step + 1 )  % 5 )
    //  return;

    int rank;
    MPI_Comm_rank(comm, &rank);

#if APOLLO_COLLECTIVE_TRAINING
    int num_measures = 0;
    for( auto &it: regions ) {
        Region *reg = it.second;
        num_measures += reg->reduceBestPolicies();
    }

    int measure_size = get_measure_size(num_features, comm);

    char *sendbuf = (char *)malloc( num_measures  * measure_size );
    //std::cout << "measure_size: " << measure_size << ", total: " << measure_size * num_measures << std::endl;
    int offset = 0;
    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Region *reg = it->second;
        int reg_num_measures = reg->best_policies.size();
        reg->packMeasurements(sendbuf + offset, reg_num_measures * measure_size, comm);
        offset += reg_num_measures * measure_size;
    }

    int num_ranks;
    MPI_Comm_size(comm, &num_ranks);
    //std::cout << "num_ranks: " << num_ranks << std::endl;

    int num_measures_per_rank[ num_ranks ];

    MPI_Allgather( &num_measures, 1, MPI_INT, &num_measures_per_rank, 1, MPI_INT, comm);

    //std::cout << "MEASURES:" ;
    //for(int i = 0; i < num_ranks; i++) {
    //  std::cout << i << ":" << num_measures_per_rank[i] << ", ";
    //}
    //std::cout << std::endl;

    int total_measures = 0;
    for(int i = 0; i < num_ranks; i++) {
        total_measures += num_measures_per_rank[i];
    }

    char *recvbuf = (char *) malloc( total_measures * measure_size );
    int recv_size_per_rank[ num_ranks ];

    for(int i = 0; i < num_ranks; i++)
        recv_size_per_rank[i] = num_measures_per_rank[i] * measure_size;

    int disp[ num_ranks ];
    disp[0] = 0;
    for(int i = 1; i < num_ranks; i++) {
        disp[i] = disp[i-1] + recv_size_per_rank[i-1];
    }

    //std::cout <<"DISP:";
    //for(int i = 0; i < num_ranks; i++) {
      //std::cout << i << ":" << disp[i] << ", ";
    //}
    //std::cout << std::endl;

    MPI_Allgatherv( sendbuf, num_measures * measure_size, MPI_PACKED, \
            recvbuf, recv_size_per_rank, disp, MPI_PACKED, comm );

    //std::cout << "BYTES TRANSFERRED: " << num_measures * measure_size << std::endl;

    //std::cout << "Rank " << rank << " TOTAL_MEASURES: " << total_measures << std::endl;
  
    for(int i = 0; i < total_measures; i++) {
        int pos = 0;
        int rank;
        std::vector<float> feature_vector;
        int policy_index;
        char region_name[64];
        int exec_count;
        double time_avg;
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &rank, 1, MPI_INT, comm);
        for(int j = 0; j < num_features; j++)  {
            float value;
            MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &value, 1, MPI_FLOAT, comm);
            feature_vector.push_back( value );
        }
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &policy_index, 1, MPI_INT, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, region_name, 64, MPI_CHAR, comm);
        //MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &exec_count, 1, MPI_INT, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &time_avg, 1, MPI_DOUBLE, comm);
        // TODO: use Region::best_policies with per region model to avoid
        // reference in apollo. Also, create new Regions if unseen locally
        // and store best_policies found to train model if executed. Simplify
        // Region constructor in RAJA to remove all arguments (unnecessary)
#if APOLLO_GLOBAL_MODEL
        auto &best_policies_ref = best_policies;
#else
        auto &best_policies_ref = best_policies[ region_name ];
        // TODO: forget or not training policies
        //best_policies[ reg->name ].clear();
#endif
 
        auto iter =  best_policies_ref.find( feature_vector );
        if( iter ==  best_policies_ref.end() ) {
            //std::cout << "--- INSERT ---" << std::endl;
            //std::cout << "" << feature_vector[0] << "]: " << "( " << policy_index \
            //    << ", " << time_avg << " )" << std::endl;
            //std::cout << "~~~~~~~~~~~" << std::endl;

             best_policies_ref.insert( { feature_vector, { policy_index, time_avg } } );
        }
        else {
            //std::cout << "--- REPLACE ---" << std::endl;
            //std::cout <<  feature_vector[0] << " > " << time_avg << std::endl;
            // Key exists
            if(  best_policies_ref[ feature_vector ].second > time_avg ) {
                //std::cout << "--- REPLACE ---" << std::endl;
                //std::cout << "Prev " << feature_vector[0] << "]: " << "( " << best_policies[ feature_vector ].first \
                //    << ", " <<  feature_vector ].second << " )" << std::endl;
                //std::cout << "Replaced " << feature_vector[0] << "]: " << "( " << policy_index \
                //    << ", " << time_avg << " )" << std::endl;
                //std::cout << "~~~~~~~~~~~" << std::endl;

                best_policies_ref[ feature_vector ] = { policy_index, time_avg };
            }
        } 

        //std::cout << "rank, region_name, num_elements, policy_index, time_avg" << std::endl;
        //std::cout << rank << ", " << region_name << ", " << (int)feature_vector[0] << ", " \
        //  << policy_index << ", " << time_avg << std::endl;

        //std::cout << "=== BEST POLICIES " << region_name << " ===" << std::endl;
        //for( auto &b : best_policies_ref ) {
        //    std::cout << "[ " << (int)b.first[0] << " ]: P:" \
        //        << b.second.first << " T: " << b.second.second << " | ";
        //}
        //std::cout << "._" << std::endl;
        //std::cout << "~~~~~~~~~" << std::endl;

    }

    //std::cout << "================== END ==================" << std::endl;
    //std::cout.close();

    free( sendbuf );
    free( recvbuf );
#else // APOLLO_LOCAL_TRAINING

#if APOLLO_GLOBAL_MODEL
    // TODO: forget or not training best_policies?
    //best_policies.clear();
#endif

    // Find best policies per region to train
    for( auto &it: regions ) {
        Region *reg = it.second;
#if APOLLO_GLOBAL_MODEL
            auto &best_policies_ref = best_policies;
#else
            auto &best_policies_ref = best_policies[ reg->name ];
            // TODO: forget or not training policies
            //best_policies[ reg->name ].clear();
#endif

        //std::cout << "=== MEASURE " << reg->name << " ===" << std::endl;
        for( auto &m : reg->measures ) {
            std::vector< float > feature_vector = m.first.first;
            int policy_index = m.first.second;
            double time_avg = m.second->time_total / m.second->exec_count;

            auto iter = best_policies_ref.find( feature_vector );
            if( iter == best_policies_ref.end() ) {
                //std::cout << "--- INSERT ---" << std::endl;
                //std::cout << "[" << feature_vector[0] << "]: " << "( " << policy_index \
                //    << ", " << time_avg << " )" << std::endl;
                //std::cout << "~~~~~~~~~~~" << std::endl;

                best_policies_ref.insert( { feature_vector, { policy_index, time_avg } } );
            }
            else {
                //std::cout << "--- REPLACE ---" << std::endl;
                //std::cout <<  feature_vector ].second << " > " << time_avg << std::endl;
                // Key exists
                if( best_policies_ref[ feature_vector ].second > time_avg ) {
                    //std::cout << "--- REPLACE ---" << std::endl;
                    //std::cout << "Prev [" << feature_vector[0] << "]: " \
                    //    << "( " << best_policies[ reg->name ][ feature_vector ].first \
                    //    << ", " << best_policies[ reg->name ][ feature_vector ].second \
                    //    << " )" << std::endl;
                    //std::cout << " -> [" << feature_vector[0] << "]: " << "( " << policy_index \
                    //    << ", " << time_avg << " )" << std::endl;
                    //std::cout << "~~~~~~~~~~~" << std::endl;

                    best_policies_ref[ feature_vector ] = { policy_index, time_avg };
                }
                //else if( best_policies[ reg->name ][ feature_vector ].second < ( .1*time_avg ) ){
                //    if ( best_policies[ reg->name ][ feature_vector ].first == policy_index )
                //        std::cout << "SUSPICIOUS case [ " << feature_vector[0] << " ]: " \
                //            << "( " << policy_index << " ) prev: " \
                //            << best_policies[ reg->name ][ feature_vector ].second \
                //            << " vs new: " << time_avg << std::endl;
                //}
            }

            //std::cout \
            //    << "[ " << static_cast<int>(feature_vector[0]) << " ]: " << policy_index << " -> " \
            //    << time_avg << ", ";
        }
        //std::cout << ".-" << std::endl;
        //std::cout << "=== BEST POLICIES " << reg->name << " ===" << std::endl;
        //for( auto &b : best_policies_ref ) {
        //    std::cout << "[ " << (int)b.first[0] << " ]: P:" \
        //        << b.second.first << " T: " << b.second.second << " | ";
        //}
        //std::cout << "._" << std::endl;
        //std::cout << "~~~~~~~~~" << std::endl;
    }
#endif

#if APOLLO_GLOBAL_MODEL
    // =============================== APOLLO_GLOBAL_MODEL ==================================================== //
    std::vector< std::vector<float> > train_features;
    std::vector< int > train_responses;

    //std::cout << "GLOBAL TRAINING " << std::endl;
    for(auto &it : best_policies) {
        train_features.push_back( it.first );
        train_responses.push_back( it.second.first );
        //std::cout << "best_policies[ " << (int)it.first[0] << " ]: "  \
        //    << "( " << it.second.first << ", " << it.second.second << " ) " << std::endl;
    }

    //std::cout << "~~~~~~~~~~~~~" << std::endl;

    //std::cout << "ONE GLOBAL TREE" << std::endl; //ggout
    std::shared_ptr<DecisionTree> gTree  = std::make_shared<DecisionTree>(
            num_policies, train_features, train_responses );

    //gTree->store( "dtree." + std::to_string( assign_to_step ) + ".yaml" );

    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Region *reg = it->second;

        // TODO: forget about old measures or not?
        //reg->measures.clear();

        reg->model = gTree;

    }

    // =============================================================================================== //
#else // APOLLO_REGION_MODEL
    for(auto it = regions.begin(); it != regions.end(); ++it) {
        std::vector< std::vector<float> > train_features;
        std::vector< int > train_responses;

        Region *reg = it->second;

        // TODO: forget about old measures or not?
        reg->measures.clear();

        // If the training data includes this region, update
        if( best_policies.find( reg->name ) != best_policies.end() ) {
            for(auto &it2 : best_policies[ reg->name ]) {
                train_features.push_back( it2.first );
                train_responses.push_back( it2.second.first );
            }

            //std::cout << "TRAIN TREE region " << reg->name << std::endl; //ggout
            reg->model = std::make_unique<DecisionTree>( 
                    num_policies, 
                    train_features, 
                    train_responses );
        }     
    }
#endif

    //std::cout << "=== FLUSH rank: " << rank \
    //    << " step: " << assign_to_step \
    //    << " best_policies size: " << best_policies.size() << " ===" << std::endl; //ggout

    return;
}

void
Apollo::setFeature(float value)
{
    features.push_back( value );
    return;
}

Apollo::Region *
Apollo::region(const char *regionName)
{
    auto search = regions.find(regionName);
    if (search != regions.end()) {
        return search->second;
    } else {
        return NULL;
    }

}
