
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

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <limits>

#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/Region.h"

#include <mpi.h>
#include <sys/time.h>

#define GET_TIME(__now)                                     \
{                                                           \
    struct timeval t;                                       \
    gettimeofday(&t, NULL);                                 \
    __now = (double)(t.tv_sec + (t.tv_usec/1e6));           \
}

int
Apollo::Region::getPolicyIndex(void)
{
    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->getPolicyIndex() called"
                        " while NOT inside the region. Please call"
                        " region->begin() first so the model has values to use"
                        " when selecting a policy. (region->name == %s)\n", name);
        fflush(stderr);
    }

    //double evaluation_time_start;
    //double evaluation_time_stop;
    //double evaluation_time_total;
    //SOS_TIME(evaluation_time_start);

    //int choice = mw->requestPolicyIndex();
    // TODO: fix getting policy from model
    int choice = 0;
    if (choice != current_policy) {
        apollo->setFeature("policy_index", (double) choice);
        //std::cout << "Change policy " << current_policy << " -> " << choice << " region " << name << std::endl; //ggout
        //std::cout.flush(); //ggout
    } else {
        //std::cout << "No policy change for region " << name << ", policy " << current_policy << std::endl; //gout
    }
#if 0
    if(choice >=2 && choice <=7) {
        //std::cout << "Region " << name <<" DYNAMIC " << choice << std::endl; //ggout
        choice = 7; //ggout
    }
    else if(choice >=8 && choice <=13) {
        //std::cout << "Region " << name <<" GUIDED " << choice << std::endl; //ggout
        choice = 13;
    }
    else if(choice >=14 && choice <=19) {
        choice = 19;
    }
#endif
    current_policy = choice;
    /*if(choice < 0 || choice > 20) {
        std::cout << "Region " << name <<" invalid choice " << choice << std::endl; //ggout
        abort();
    }*/
    //if(choice !=0 ) //ggout
    //    std::cout << "v4 Region " << name << " policy " << current_policy << std::endl;

    //SOS_TIME(evaluation_time_stop);
    //evaluation_time_total = evaluation_time_stop - evaluation_time_start;
    //log("getPolicyIndex took ", evaluation_time_total, " seconds.\n");

    return choice;
}


Apollo::Region::Region(
        const char  *regionName,
        int          numAvailablePolicies)
{
    apollo = Apollo::instance();
    strncpy(name, regionName, sizeof(name)-1 );
    name[ sizeof(name)-1 ] = '\0';

    current_policy            = 0;
    currently_inside_region   = false;

    // TODO: bootstrap model
    //model_wrapper = new Apollo::ModelWrapper(this, numAvailablePolicies);
    //model_wrapper->configure("");

    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    //std::cout.flush(); //ggout
    apollo->regions.insert({name, this});

    return;
}

Apollo::Region::~Region()
{
    if (currently_inside_region) {
        this->end();
    }

    /*if (name != NULL) {
        free(name);
        name = NULL;
    }*/

    return;
}


void
Apollo::Region::begin()
{
    if (currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->begin() called"
                        " while already inside the region. Please call"
                        " region->end() first to avoid unintended"
                        " consequences. (region->name == %s)\n",
                        name);
        fflush(stderr);
    }
    currently_inside_region = true;

    // NOTE: Features are tracked globally within the process.
    //       Apollo semantics require that region.begin/end calls happen
    //       from the top-level process thread, they are not encountered
    //       within a parallel code region.
    //
    //       We update this value here in case another region used a different
    //       policy index, in case this value gets looked up between this
    //       call to region.begin and our model being newly evaluated by
    //       region.getPolicyIndex ...
    //
    apollo->setFeature("policy_index", (double) current_policy);

    GET_TIME(current_exec_time_begin);
    return;
}

void
Apollo::Region::end()
{
    GET_TIME(current_exec_time_end);

    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->end() called"
                        " while NOT inside the region. Please call"
                        " region->begin(step) first to avoid unintended"
                        " consequences. (region->name == %s)\n", name);
        fflush(stderr);
    }
    currently_inside_region = false;

    // In case this was changed by the DecisionTree after the begin() call...
    apollo->setFeature("policy_index", (double) current_policy);

    Apollo::Region::Measure *time = nullptr;
    auto iter = measures.find(apollo->features);
    if (iter == measures.end()) {
        time = new Apollo::Region::Measure;
        time->exec_count = 0;
        time->time_total = 0.0;
    } else {
        time = iter->second;
    }

    time->exec_count++;
    time->time_total += (current_exec_time_end - current_exec_time_begin);

    if (iter == measures.end()) {
        std::vector<Apollo::Feature> feat_copy = apollo->features;
        measures.insert({std::move(feat_copy), time});
    }

    return;
}

void
Apollo::Region::packMeasurements(char *buf, int size, MPI_Comm comm) {
    int pos = 0;

    for (auto iter_measure = measures.begin();
             iter_measure != measures.end();   iter_measure++) {

        const std::vector<Apollo::Feature>& these_features = iter_measure->first;
        Apollo::Region::Measure                  *time_set = iter_measure->second;

        int rank;
        MPI_Comm_rank( comm, &rank );
        MPI_Pack( &rank, 1, MPI_INT, buf, size, &pos, comm);
        std::cout << "------------------" << std::endl;
        std::cout << "rank,"<< rank << " pos: " << pos << std::endl;
        for (Apollo::Feature ft : these_features) {
            MPI_Pack( &ft.value, 1, MPI_DOUBLE, buf, size, &pos, comm );
            std::cout << ft.name <<","<< ft.value << " pos: " << pos << std::endl;
        }
        assert( these_features.size() == 3 );

        // XXX: use 64 bytes fixed for region_name
        MPI_Pack( name, 64, MPI_CHAR, buf, size, &pos, comm );
        std::cout << "region_name," << name << " pos: " << pos << std::endl;
        MPI_Pack( &time_set->exec_count, 1, MPI_INT, buf, size, &pos, comm );
        std::cout << "exec_count," << time_set->exec_count << " pos: " << pos << std::endl;
        assert( exec_count > 0 );
        double avg_time = (time_set->time_total / time_set->exec_count);
        assert( avg_time > 0 );
        MPI_Pack( &avg_time, 1, MPI_DOUBLE, buf, size, &pos, comm );

        std::cout << "time_avg," << avg_time << " pos: " << pos << std::endl;

        std::cout << "~~~~~~~~~~~~~~~~~" << std::endl;
    }

    return;
}

