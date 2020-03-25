#ifndef APOLLO_H
#define APOLLO_H

#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <type_traits>
#include <list>
#include <vector>

#include <omp.h>
#include <mpi.h>

#include "apollo/Logging.h"

// TODO: create a better configuration method
#define APOLLO_COLLECTIVE_TRAINING 0

#define APOLLO_GLOBAL_MODEL 1
#if APOLLO_GLOBAL_MODEL
#define APOLLO_REGION_MODEL 0
#else
#define APOLLO_REGION_MODEL 1
#endif

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

        //
        class Region;
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
        double  getFeature(std::string ft_name);

        // NOTE(chad): We default to walk_distance of 2 so we can
        //             step out of this method, then step out of
        //             our RAJA policy template, and get to the
        //             module name and offset where that template
        //             has been instantiated in the application code.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

        Region *region(const char *regionName);
        //
        bool isOnline();
        //
        void flushAllRegionMeasurements(int assign_to_step);
    private:
        MPI_Comm comm;
        std::map<const char *, Region *> regions;
        Apollo();
        // Key: region name, value: map key: num_elements, value: policy_index, time_avg
#if APOLLO_GLOBAL_MODEL
        std::map< std::vector< float >, std::pair< int, double > > best_policies;
#else // APOLLO_REGION_MODEL
        std::map< std::string,
            std::map< std::vector< float >, std::pair< int, double > > > best_policies;
#endif

}; //end: Apollo

inline const char*
safe_getenv(
        const char *var_name,
        const char *use_this_if_not_found,
        bool        silent=false)
{
    char *c = getenv(var_name);
    if (c == NULL) {
        if (not silent) {
            log("Looked for ", var_name, " with getenv(), found nothing, using '", \
                use_this_if_not_found, "' (default) instead.");
        }
        return use_this_if_not_found;
    } else {
        return c;
    }
}

#endif
