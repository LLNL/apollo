#ifndef APOLLO_H
#define APOLLO_H

#include <string>
#include <map>
#include <vector>

#include <omp.h>
#include <mpi.h>

#include "apollo/Config.h"
#include "apollo/Logging.h"

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
        //
        // XXX: assumes features are the same globally for all regions
        std::vector<float>            features;
        int                       num_features;
        int                       num_policies;
        //
        // Precalculated at Apollo::Init from evironment variable strings to
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
        int numThreads;  // <-- how many to use / are in use
        //
        void    setFeature(float value);

        // NOTE(chad): We default to walk_distance of 2 so we can
        //             step out of this method, then step out of
        //             our RAJA policy template, and get to the
        //             module name and offset where that template
        //             has been instantiated in the application code.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

        void flushAllRegionMeasurements(int assign_to_step);
    private:
        MPI_Comm comm;
        Apollo();
        void gatherReduceCollectiveTrainingData();
        // Key: region name, value: region raw pointer
        std::map<std::string, Apollo::Region *> regions;
        // Key: region name, value: map key: num_elements, value: policy_index, time_avg
        std::map< std::vector< float >, std::pair< int, double > > best_policies_global;
}; //end: Apollo

#endif
