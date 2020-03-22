
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
#include <iostream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <unordered_map>
#include <algorithm>

#include <omp.h>

#include "CallpathRuntime.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/Region.h"
//
#include "util/Debug.h"
//
//ggout
//#include <opencv2/ml.hpp>
//using namespace cv;
//using namespace cv::ml;


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
    walk_distance = 2; //ggout
    CallpathRuntime *cp = (CallpathRuntime *) callpath_ptr;
    // Set up this Apollo::Region for the first time:       (Runs only once)
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

void
Apollo::flushAllRegionMeasurements(int assign_to_step)
{
    int num_measures = 0;
    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Apollo::Region *reg = it->second;
        num_measures += reg->measures.size();
    }

    int rank;
    MPI_Comm_rank(comm, &rank);
    std::ofstream ofile("rank-" + std::to_string(rank) + "-measures-" + std::to_string(assign_to_step) + std::string(".txt") );
    // 3 fixed features: region_name, exec_count, time_avg
    int size = 0, measure_size = 0;
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    MPI_Pack_size( features.size(), MPI_DOUBLE, comm, &size);
    assert( features.size() == 3 );
    measure_size += size;
    MPI_Pack_size( 64, MPI_CHAR, comm, &size);
    measure_size += size;
    MPI_Pack_size( 1, MPI_INT, comm, &size);
    measure_size += size;
    MPI_Pack_size( 1, MPI_DOUBLE, comm, &size);
    measure_size += size;

    char *sendbuf = (char *)malloc( num_measures  * measure_size );
    ofile << "measure_size: " << measure_size << ", total: " << measure_size * num_measures << std::endl;
    int offset = 0;
    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Apollo::Region *reg = it->second;
        reg->packMeasurements(sendbuf + offset, reg->measures.size() * measure_size, comm);
        offset += reg->measures.size() * measure_size;
    }

    int num_ranks;
    MPI_Comm_size(comm, &num_ranks);
    ofile << "num_ranks: " << num_ranks << std::endl;

    int num_measures_per_rank[ num_ranks ];

    MPI_Allgather( &num_measures, 1, MPI_INT, &num_measures_per_rank, 1, MPI_INT, comm);

    ofile << "MEASURES:" ;
    for(int i = 0; i < num_ranks; i++) {
        ofile << i << ":" << num_measures_per_rank[i] << ", ";
    }
    ofile << std::endl;

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
    ofile <<"DISP:";
    for(int i = 0; i < num_ranks; i++) {
        ofile << i << ":" << disp[i] << ", ";
    }
    ofile << std::endl;

    MPI_Allgatherv( sendbuf, num_measures * measure_size, MPI_PACKED, \
            recvbuf, recv_size_per_rank, disp, MPI_PACKED, comm );

    ofile << "Rank " << rank << " FLUSH ALL MEASUREMENTS " << std::endl; //ggout

    ofile << "Rank " << rank << " TOTAL_MEASURES: " << total_measures << std::endl;
    ofile << "rank, ";
    for (Apollo::Feature &ft : features)
        ofile << ft.name << ", ";
    ofile<<"region_name, exec_count, time_avg" << std::endl;

    //std::map<double, std::pair< int, double > > samples; //ggout

    for(int i = 0; i < total_measures; i++) {
        int pos = 0;
        int rank;
        double policy_index, num_elements, num_threads;
        char region_name[64];
        int exec_count;
        double time_avg;
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &rank, 1, MPI_INT, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &policy_index, 1, MPI_DOUBLE, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &num_elements, 1, MPI_DOUBLE, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &num_threads, 1, MPI_DOUBLE, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, region_name, 64, MPI_CHAR, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &exec_count, 1, MPI_INT, comm);
        MPI_Unpack(&recvbuf[ i * measure_size ], measure_size, &pos, &time_avg, 1, MPI_DOUBLE, comm);

        /*if( samples.insert( std::pair( num_elements, std::pair( policy_index, time_avg ) ) ).second == false ) {
            samples[ num_elements ] = ( 
        }*/ //ggout

        ofile << rank << ", " << policy_index << ", " << num_elements << ", " << num_threads << ", " << region_name
            << ", " << exec_count << ", " << time_avg << std::endl;
    }

    ofile << "================== END ==================" << std::endl;
    ofile.close();

    free( sendbuf );
    free( recvbuf );

    for(auto it = regions.begin(); it != regions.end(); ++it) {
        Apollo::Region *reg = it->second;
        reg->measures.clear();
    }

    // One model for all regions

    return;
}


void
Apollo::setFeature(std::string set_name, double set_value)
{
    bool found = false;

    for (int i = 0; i < features.size(); ++i) {
        if (features[i].name == set_name) {
            found = true;
            features[i].value = set_value;
            break;
        }
    }

    if (not found) {
        Apollo::Feature f;
        f.name  = set_name;
        f.value = set_value;

        features.push_back(std::move(f));
    }

    return;
}

double
Apollo::getFeature(std::string req_name)
{
    double retval = 0.0;

    for(Apollo::Feature ft : features) {
        if (ft.name == req_name) {
            retval = ft.value;
            break;
        }
    };

    return retval;
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
