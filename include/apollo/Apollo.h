#ifndef APOLLO_H
#define APOLLO_H

#include <fstream>
#include <string>
#include <map>
#include <vector>

#include <omp.h>

#include "apollo/Config.h"

//TODO(cdw): Convert 'Apollo' into a namespace and convert this into
//           a 'Runtime' class.
class Apollo
{
    public:
       ~Apollo();
        // disallow copy constructor
        Apollo(const Apollo&) = delete;
        Apollo& operator=(const Apollo&) = delete;

        static Apollo* instance(void) noexcept {
            static Apollo the_instance;
            return &the_instance;
        }

        class Region;
        struct RegionContext;
        struct CallbackDataPool;

        //TODO(cdw): This is serving as an override that is defined by an
        //           environment variable.  Apollo::Region's are able to
        //           have different policy counts, so a global setting here
        //           should indicate that it is an override, or find
        //           a better place to live.  Leaving it for now, as a low-
        //           priority task.
        int  num_policies;

        // These are convienience values that get precalculated
        // at Apollo::Init from evironment variables / strings to
        // facilitate quick calculations during model evaluation later.
        int numNodes;
        int numCPUsOnNode;
        int numProcs;
        int numProcsPerNode;
        int numThreadsPerProcCap;
        omp_sched_t ompDefaultSchedule;
        int         ompDefaultNumThreads;
        int         ompDefaultChunkSize;
        //
        int mpiSize;   // 1 if no MPI
        int mpiRank;   // 0 if no MPI
        //
        int numThreads;  // <-- how many to use / are in use

        // NOTE(chad): We default to walk_distance of 2 so we can
        //             step out of this method, then step out of
        //             some portable policy template, and get to the
        //             module name and offset where that template
        //             has been instantiated in the application code.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

        void flushAllRegionMeasurements(int step);

    private:
        Apollo();
        //
        void gatherReduceCollectiveTrainingData(int step);
        // Key: region name, value: region raw pointer
        std::map<std::string, Apollo::Region *> regions;
        // Key: region name, value: map key: num_elements, value: policy_index, time_avg
        std::map< std::vector< float >, std::pair< int, double > > best_policies_global;
        // Count total number of region invocations
        unsigned long long region_executions;
}; //end: Apollo

extern "C" {
 void *__apollo_region_create(int num_features, char *id, int num_policies);
 void __apollo_region_begin(Apollo::Region *r);
 void __apollo_region_end(Apollo::Region *r);
 void __apollo_region_set_feature(Apollo::Region *r, float feature);
 int __apollo_region_get_policy(Apollo::Region *r);
}

#endif
