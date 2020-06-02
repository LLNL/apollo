#ifndef APOLLO_H
#define APOLLO_H

#include <fstream>
#include <string>
#include <map>
#include <vector>

#include <omp.h>

#ifdef APOLLO_ENABLE_MPI
#include <mpi.h>
#endif //APOLLO_ENABLE_MPI

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

        //////////
        //
        //  TODO(cdw): Move this into a dedicated 'Trace' class during code refactor.
        bool          traceEnabled;
        bool          traceEmitOnline;
        bool          traceEmitAllFeatures;
        bool          traceOutputIsActualFile;
        std::string   traceOutputFileName;
        std::ofstream traceOutputFileHandle;
        //
        typedef std::tuple<
            double,
            std::string,
            int,
            std::string,
            int,
            int,
            int,
            double,
            std::string
            > TraceLine_t;
        typedef std::vector<TraceLine_t> TraceVector_t;
        //
        void storeTraceLine(TraceLine_t &t);
        //
        void writeTraceHeaderImpl(std::ostream &sink);
        void writeTraceHeader(void);
        void writeTraceLineImpl(TraceLine_t &t, std::ostream &sink);
        void writeTraceLine(TraceLine_t &t);
        void writeTraceVector(void);
        //
        //////////


        //TODO(cdw): This is serving as an override that is defined by an
        //           environment variable.  Apollo::Region's are able to
        //           have different policy counts, so a global setting here
        //           should indicate that it is an override, or find
        //           a better place to live.  Leaving it for now, as a low-
        //           priority task.
        int  num_policies;

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
        int mpiSize;
        int mpiRank;
        //
        int numThreads;  // <-- how many to use / are in use

        // NOTE(chad): We default to walk_distance of 2 so we can
        //             step out of this method, then step out of
        //             our RAJA policy template, and get to the
        //             module name and offset where that template
        //             has been instantiated in the application code.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

        void flushAllRegionMeasurements(int step);

    private:
        Apollo();
        //
        TraceVector_t trace_data;
        //
        void packMeasurements(char *buf, int size, void *_reg);
        void gatherReduceCollectiveTrainingData(int step);
        // Key: region name, value: region raw pointer
        std::map<std::string, Apollo::Region *> regions;
        // Key: region name, value: map key: num_elements, value: policy_index, time_avg
        std::map< std::vector< float >, std::pair< int, double > > best_policies_global;
}; //end: Apollo

#endif
