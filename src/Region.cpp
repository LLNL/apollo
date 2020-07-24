
// Copyright (c) 2020, Lawrence Livermore National Security, LLC.
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
#include <fstream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <utility>

#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/Region.h"
#include "apollo/Logging.h"
#include "apollo/ModelFactory.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif //ENABLE_MPI

int
Apollo::Region::getPolicyIndex(void)
{
#if VERBOSE_DEBUG
    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->getPolicyIndex() called"
                        " while NOT inside the region. Please call"
                        " region->begin() first so the model has values to use"
                        " when selecting a policy. (region->name == %s)\n", name);
        fflush(stderr);
    }

    assert( currently_inside_region );
#endif

    int choice = model->getIndex( features );

    if( Config::APOLLO_TRACE_POLICY ) {
        std::stringstream trace_out;
        int rank;
#ifdef ENABLE_MPI
        MPI_Comm_rank( apollo->comm, &rank );
#else
        rank = 0;
#endif //ENABLE_MPI
        trace_out << "Rank " << rank \
            << " region " << name \
            << " model " << model->name \
            << " features [ ";
        for(auto &f: features)
            trace_out << (int)f << ", ";
        trace_out << "] policy " << choice << std::endl;
        std::cout << trace_out.str();
        std::ofstream fout( "rank-" + std::to_string(rank) + "-policies.txt", std::ofstream::app );
        fout << trace_out.str();
        fout.close();
    }

#if 0
    if (choice != current_policy) {
        std::cout << "Change policy " << current_policy \
            << " -> " << choice << " region " << name \
            << " training " << model->training \
            << " features: "; \
            for(auto &f : apollo->features) \
                std::cout << (int)f << ", "; \
            std::cout << std::endl; //ggout
    } else {
        //std::cout << "No policy change for region " << name << ", policy " << current_policy << std::endl; //gout
    }
#endif
    current_policy = choice;
    //log("getPolicyIndex took ", evaluation_time_total, " seconds.\n");
    return choice;
}


Apollo::Region::Region(
        const int num_features,
        const char  *regionName,
        int          numAvailablePolicies)
    :
        num_features(num_features)
{
    apollo = Apollo::instance();
    if( Config::APOLLO_NUM_POLICIES ) {
        apollo->num_policies = Config::APOLLO_NUM_POLICIES;
    }
    else {
        apollo->num_policies = numAvailablePolicies;
    }

    strncpy(name, regionName, sizeof(name)-1 );
    name[ sizeof(name)-1 ] = '\0';

    current_policy            = -1;
    currently_inside_region   = false;


    if( Config::APOLLO_LOAD_MODEL != "" ) {
        model = ModelFactory::loadDecisionTree( apollo->num_policies, Config::APOLLO_LOAD_MODEL );
    } else {
        // TODO use best_policies to train a model for new region for which there's training data
        size_t pos = Config::APOLLO_INIT_MODEL.find(",");
        std::string model_str = Config::APOLLO_INIT_MODEL.substr(0, pos);
        if( "Static" == model_str ) {
            int policy_choice = std::stoi( Config::APOLLO_INIT_MODEL.substr( pos+1 ) );
            if( policy_choice < 0 || policy_choice >= numAvailablePolicies ) {
                std::cerr << "Invalid policy_choice " << policy_choice << std::endl;
                abort();
            }
            model = ModelFactory::createStatic( apollo->num_policies, policy_choice );
            //std::cout << "Model Static policy " << policy_choice << std::endl;
        }
        else if( "Random" == model_str ) {
            model = ModelFactory::createRandom( apollo->num_policies );
            //std::cout << "Model Random" << std::endl;
        }
        else if( "RoundRobin" == model_str ) {
            model = ModelFactory::createRoundRobin( apollo->num_policies );
            //std::cout << "Model RoundRobin" << std::endl;
        }
        else {
            std::cerr << "Invalid model env var: " + Config::APOLLO_INIT_MODEL << std::endl;
            abort();
        }
    }
    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    const auto ret = apollo->regions.insert( { name, this } );

    return;
}

Apollo::Region::Region(
        const int num_features,
        const char  *regionName,
        int          numAvailablePolicies,
        std::string  loadModelFromThisYamlFile)
    :
        num_features(num_features)
{
    apollo = Apollo::instance();
    if( Config::APOLLO_NUM_POLICIES ) {
        apollo->num_policies = Config::APOLLO_NUM_POLICIES;
    }
    else {
        apollo->num_policies = numAvailablePolicies;
    }

    strncpy(name, regionName, sizeof(name)-1 );
    name[ sizeof(name)-1 ] = '\0';

    current_policy            = -1;
    currently_inside_region   = false;


    model = ModelFactory::loadDecisionTree( apollo->num_policies, loadModelFromThisYamlFile );
    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    const auto ret = apollo->regions.insert( { name, this } );

    return;
}

Apollo::Region::~Region()
{
    if (currently_inside_region) {
        this->end();
    }

    return;
}


void
Apollo::Region::begin()
{
#if VERBOSE_DEBUG
    if (currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->begin() called"
                        " while already inside the region. Please call"
                        " region->end() first to avoid unintended"
                        " consequences. (region->name == %s)\n",
                        name);
        fflush(stderr);
    }

    assert( !currently_inside_region );
#endif


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

    current_exec_time_begin = std::chrono::steady_clock::now();
    return;
}




void
Apollo::Region::end(double duration)
{

#if VERBOSE_DEBUG
    if (not currently_inside_region) {
        fprintf(stderr, "== APOLLO: [WARNING] region->end() called"
                        " while NOT inside the region. Please call"
                        " region->begin(step) first to avoid unintended"
                        " consequences. (region->name == %s)\n", name);
        fflush(stderr);
    }
    assert( currently_inside_region );
#endif
    currently_inside_region = false;


    // TODO reduce overhead, move time calculation to reduceBestPolicies?
    // TODO buckets of features?
    //const int bucket = 10;
    //for(auto &it : apollo->features) {
    //    //std::cout << "feature: " << it;
    //    int idiv = int(it) / bucket;
    //    it = ( idiv + 1 )* bucket;
    //    //std::cout << " -> " << it;
    //    //std::cout << std::endl;
    //}

    auto iter = measures.find( { features, current_policy } );
    if (iter == measures.end()) {
        iter = measures.insert(std::make_pair( std::make_pair( features, current_policy ),
                std::move( std::make_unique<Apollo::Region::Measure>(1, duration) )
                ) ).first;
    } else {
        iter->second->exec_count++;
        iter->second->time_total += duration;
    }

    //std::cout << "=== INSERT MEASURE ===" << std::endl; \
    std::cout << name << ": " << "[ "; \
        for(auto &f : apollo->features) { \
            std::cout << (int)f << ", "; \
        } \
    std::cout << " ]: " \
        << current_policy << " = ( " << iter->second->exec_count \
        << ", " << iter->second->time_total << " ) " << std::endl; \
    std::cout << ".-" << std::endl;


    features.clear();

    return;
}

void
Apollo::Region::end(void)
{
    current_exec_time_end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(current_exec_time_end - current_exec_time_begin).count();
    end(duration);
}


int
Apollo::Region::reduceBestPolicies(int step)
{
    std::stringstream trace_out;
    int rank;
    if( Config::APOLLO_TRACE_MEASURES ) {
#ifdef ENABLE_MPI
        MPI_Comm_rank(apollo->comm, &rank);
#else
        rank = 0;
#endif //ENABLE_MPI
        trace_out << "=================================" << std::endl \
            << "Rank " << rank << " Region " << name << " MEASURES "  << std::endl;
    }
    for (auto iter_measure = measures.begin();
            iter_measure != measures.end();   iter_measure++) {

        const std::vector<float>& feature_vector = iter_measure->first.first;
        const int policy_index                   = iter_measure->first.second;
        auto                           &time_set = iter_measure->second;

        if( Config::APOLLO_TRACE_MEASURES ) {
            trace_out << "features: [ ";
            int mul = 1;
            for(auto &f : feature_vector ) { \
                trace_out << (int)f << ", ";
                mul *= f;
            }
            trace_out << " = " << mul << " ]: "
                << "policy: " << policy_index
                << " , count: " << time_set->exec_count
                << " , total: " << time_set->time_total
                << " , time_avg: " <<  ( time_set->time_total / time_set->exec_count ) << std::endl;
        }
        double time_avg = ( time_set->time_total / time_set->exec_count );

        auto iter =  best_policies.find( feature_vector );
        if( iter ==  best_policies.end() ) {
            best_policies.insert( { feature_vector, { policy_index, time_avg } } );
        }
        else {
            // Key exists
            if(  best_policies[ feature_vector ].second > time_avg ) {
                best_policies[ feature_vector ] = { policy_index, time_avg };
            }
        }
    }

    if( Config::APOLLO_TRACE_MEASURES ) {
        trace_out << ".-" << std::endl;
        trace_out << "Rank " << rank << " Region " << name << " Reduce " << std::endl;
        for( auto &b : best_policies ) {
            trace_out << "features: [ ";
            for(auto &f : b.first )
                trace_out << (int)f << ", ";
            trace_out << "]: P:"
                << b.second.first << " T: " << b.second.second << std::endl;
        }
        trace_out << ".-" << std::endl;
        std::cout << trace_out.str();
        std::ofstream fout("step-" + std::to_string(step) +
                "-rank-" + std::to_string(rank) + "-" + name + "-measures.txt"); \
            fout << trace_out.str();
        fout.close();
    }

    return best_policies.size();
}

void
Apollo::Region::packMeasurements(char *buf, int size) {
#ifdef ENABLE_MPI
    int pos = 0;
    int rank;
    MPI_Comm_rank( comm, &rank );

    for( auto &it : best_policies ) {
        auto &feature_vector = it.first;
        int policy_index = it.second.first;
        double time_avg = it.second.second;

        // rank
        MPI_Pack( &rank, 1, MPI_INT, buf, size, &pos, comm);
        //std::cout << "rank," << rank << " pos: " << pos << std::endl;

        // num features
        MPI_Pack( &num_features, 1, MPI_INT, buf, size, &pos, comm);
        //std::cout << "rank," << rank << " pos: " << pos << std::endl;

        // feature vector
        for (float value : feature_vector ) {
            MPI_Pack( &value, 1, MPI_FLOAT, buf, size, &pos, comm );
            //std::cout << "feature," << value << " pos: " << pos << std::endl;
        }

        // policy index
        MPI_Pack( &policy_index, 1, MPI_INT, buf, size, &pos, comm );
        //std::cout << "policy_index," << policy_index << " pos: " << pos << std::endl;
        // XXX: use 64 bytes fixed for region_name
        // region name
        MPI_Pack( name, 64, MPI_CHAR, buf, size, &pos, comm );
        //std::cout << "region_name," << name << " pos: " << pos << std::endl;
        // average time
        MPI_Pack( &time_avg, 1, MPI_DOUBLE, buf, size, &pos, comm );
        //std::cout << "time_avg," << time_avg << " pos: " << pos << std::endl;
    }
#endif //ENABLE_MPI
    return;
}


void
Apollo::Region::setFeature(float value)
{
    features.push_back( value );
    return;
}
