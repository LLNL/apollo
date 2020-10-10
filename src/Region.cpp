
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
#include <ctime>
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
Apollo::Region::getPolicyIndex(Apollo::RegionContext *context)
{
    int choice = model->getIndex( context->features );

    if( Config::APOLLO_TRACE_POLICY ) {
        std::stringstream trace_out;
        int rank;
        rank = apollo->mpiRank;
        trace_out << "Rank " << rank \
            << " region " << name \
            << " model " << model->name \
            << " features [ ";
        for(auto &f: context->features)
            trace_out << (int)f << ", ";
        trace_out << "] policy " << choice << std::endl;
        std::cout << trace_out.str();
        std::ofstream fout( "rank-" + std::to_string(rank) + "-policies.txt", std::ofstream::app );
        fout << trace_out.str();
        fout.close();
    }

#if 0
    if (choice != context->policy) {
        std::cout << "Change policy " << context->policy\
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
    context->policy = choice;
    //log("getPolicyIndex took ", evaluation_time_total, " seconds.\n");
    return choice;
}

Apollo::Region::Region(
        const int num_features,
        const char  *regionName,
        int          numAvailablePolicies)
    :
        num_features(num_features), current_context(nullptr)
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

    // TODO use best_policies to train a model for new region for which there's training data
    size_t pos = Config::APOLLO_INIT_MODEL.find(",");
    std::string model_str = Config::APOLLO_INIT_MODEL.substr(0, pos);
    if ("Static" == model_str)
    {
        int policy_choice = std::stoi(Config::APOLLO_INIT_MODEL.substr(pos + 1));
        if (policy_choice < 0 || policy_choice >= numAvailablePolicies)
        {
            std::cerr << "Invalid policy_choice " << policy_choice << std::endl;
            abort();
        }
        model = ModelFactory::createStatic(apollo->num_policies, policy_choice);
        //std::cout << "Model Static policy " << policy_choice << std::endl;
    }
    else if ("Load" == model_str)
    {
        std::string model_file = Config::APOLLO_INIT_MODEL.substr(pos + 1);
        model = ModelFactory::loadDecisionTree(apollo->num_policies, model_file);
        //std::cout << "Model Load " << model_file << std::endl;
    }
    else if ("Random" == model_str)
    {
        model = ModelFactory::createRandom(apollo->num_policies);
        //std::cout << "Model Random" << std::endl;
    }
    else if ("RoundRobin" == model_str)
    {
        model = ModelFactory::createRoundRobin(apollo->num_policies);
        //std::cout << "Model RoundRobin" << std::endl;
    }
    else
    {
        std::cerr << "Invalid model env var: " + Config::APOLLO_INIT_MODEL << std::endl;
        abort();
    }
    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    const auto ret = apollo->regions.insert( { name, this } );

    return;
}

// TODO: Is this constructor used? remove?
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

    model = ModelFactory::loadDecisionTree( apollo->num_policies, loadModelFromThisYamlFile );
    //std::cout << "Insert region " << name << " ptr " << this << std::endl;
    const auto ret = apollo->regions.insert( { name, this } );

    return;
}

Apollo::Region::~Region()
{
        this->end();

    return;
}

Apollo::RegionContext *
Apollo::Region::begin()
{
    Apollo::RegionContext *context = new Apollo::RegionContext();
    current_context = context;
    context->exec_time_begin = std::chrono::steady_clock::now();
    return context;
    }

Apollo::RegionContext *
Apollo::Region::begin(std::vector<float> features)
{
    Apollo::RegionContext *context = begin();
    context->features = features;
    return context;
}

void
Apollo::Region::end(Apollo::RegionContext *context, double metric)
{
    auto iter = measures.find({context->features, context->policy});
    if (iter == measures.end()) {
        iter = measures.insert(std::make_pair( std::make_pair( context->features, context->policy ),
                std::move( std::make_unique<Apollo::Region::Measure>(1, metric) )
                ) ).first;
    } else {
        iter->second->exec_count++;
        iter->second->time_total += metric;
    }

    // TODO: is this code still relevant?
    if (apollo->traceEnabled) {
        // TODO(cdw): extract the correct values.
        int num_threads      = -999;
        int num_elements     = current_elem_count;
        int policy_index     = context->policy;
        std::string node_id  = "localhost";
        int comm_rank        = 0;
        //for (Apollo::Feature ft : apollo->features)
        //{
        //    if (     ft.name == "policy_index") { policy_index = (int) ft.value; }
        //    else if (ft.name == "num_threads")  { num_threads  = (int) ft.value; }
        //    else if (ft.name == "num_elements") { num_elements = (int) ft.value; }
        //}

        std::chrono::duration<double, std::milli> wall_elapsed = context->exec_time_end.time_since_epoch();
        double wall_time = wall_elapsed.count();

        std::chrono::duration<double, std::milli> loop_elapsed = context->exec_time_end - context->exec_time_begin;
        double loop_time = loop_elapsed.count();

        std::string optional_all_feature_column = "";

        // TODO(cdw): Construct a JSON of all features to emit as an extra column,
        //            in the future when we have more features.
        // NOTE.....: This works when features are a key/value map.
        //
        //if (apollo->traceEmitAllFeatures) {
        //    if (apollo->features.size() > 0) {
        //        std::stringstream ss;
        //        ss.precision(17);
        //        ss << ",\"{";
        //        for (Apollo::Feature &ft : apollo->features) {
        //            ss << "'" << ft.name << "':'" << std::fixed << ft.value << "',";
        //        }
        //        ss.seekp(-1, ss.cur);
        //        ss << "}\"";
        //        optional_all_feature_column = ss.str();
        //    } else {
        //          optional_all_feature_column = "";
        //          optional_all_feature_column = ",\"{'none':'none'}\"";
        //    }
        //}

        // TODO(cdw): ...for now, we make a quoted CSV of the elements counts
        //            for OpenMP collapsed loops.
        if (apollo->traceEmitAllFeatures)
        {
            std::stringstream ssvec;
            ssvec << "\"";
            for (auto &ft : context->features) {
                ssvec << ft;
                if (&ft != &context->features.back()) {
                    ssvec << ",";
                }
            }
            ssvec << "\"";
            optional_all_feature_column += ",";
            optional_all_feature_column += ssvec.str();
        }

        Apollo::TraceLine_t \
           t = std::make_tuple(
                wall_time,
                node_id,
                comm_rank,
                name,
                policy_index,
                num_threads,
                num_elements,
                loop_time,
                optional_all_feature_column
            );

        if (apollo->traceEmitOnline) {
            apollo->writeTraceLine(t);
        } else {
            apollo->storeTraceLine(t);
        }
    } // end: if (apollo->traceEnabled)

    apollo->region_executions++;

    if( Config::APOLLO_FLUSH_PERIOD && ( apollo->region_executions% Config::APOLLO_FLUSH_PERIOD ) == 0 ) {
        //std::cout << "FLUSH PERIOD! region_executions " << apollo->region_executions<< std::endl; //ggout
        apollo->flushAllRegionMeasurements(apollo->region_executions);
    }

    delete context;
    current_context = nullptr;

    return;
}

void
Apollo::Region::end(Apollo::RegionContext *context)
{
    context->exec_time_end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(context->exec_time_end - context->exec_time_begin).count();
    end(context, duration);
}


// DEPRECATED
int
Apollo::Region::getPolicyIndex(void)
{
    return getPolicyIndex(current_context);
}

// DEPRECATED
void
Apollo::Region::end(double metric)
{
    end(current_context, metric);
}

// DEPRECATED
void
Apollo::Region::end(void)
{
    end(current_context);
}


int
Apollo::Region::reduceBestPolicies(int step)
{
    std::stringstream trace_out;
    int rank;
    if( Config::APOLLO_TRACE_MEASURES ) {
#ifdef ENABLE_MPI
        rank = apollo->mpiRank;
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
Apollo::Region::setFeature(Apollo::RegionContext *context, float value)
{
    context->features.push_back(value);
    current_elem_count = value;
    return;
}

// DEPRECATED
void
Apollo::Region::setFeature(float value)
{
    setFeature(current_context, value);
}
