
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <chrono>
#include <memory>
#include <map>
#include <fstream>

#include "apollo/Apollo.h"
#include "apollo/PolicyModel.h"
#include "apollo/TimingModel.h"

#ifdef ENABLE_MPI
#include <mpi.h>
#endif //ENABLE_MPI

class Apollo::Region {
    public:
        Region(
                const int    num_features,
                const char   *regionName,
                int           numAvailablePolicies,
                Apollo::CallbackDataPool *callbackPool = nullptr,
                const std::string &modelYamlFile="");
        ~Region();

        typedef struct Measure {
            int       exec_count;
            double    time_total;
            Measure(int e, double t) : exec_count(e), time_total(t) {}
        } Measure;

        char     name[64];

        // DEPRECATED interface assuming synchronous execution, will be removed
        void     end();
        // lower == better, 0.0 == perfect
        void     end(double synthetic_duration_or_weight);
        int      getPolicyIndex(void);
        void     setFeature(float value);
        // END of DEPRECATED

        Apollo::RegionContext *begin();
        Apollo::RegionContext *begin(std::vector<float>);
        void end(Apollo::RegionContext *);
        void end(Apollo::RegionContext *, double);
        int  getPolicyIndex(Apollo::RegionContext *);
        void setFeature(Apollo::RegionContext *, float value);

        int idx;
        int      num_features;
        int      reduceBestPolicies(int step);
        //
        // Application specific callback data pool associated with the region, deleted by apollo.
        Apollo::CallbackDataPool *callback_pool;

        std::map<
            std::vector< float >,
            std::pair< int, double > > best_policies;

        std::map<
            std::pair< std::vector<float>, int >,
            std::unique_ptr<Apollo::Region::Measure> > measures;
        //^--Explanation: < features, policy >, value: < time measurement >

        std::unique_ptr<TimingModel> time_model;
        std::unique_ptr<PolicyModel> model;

        void collectPendingContexts();
    private:
        //
        Apollo        *apollo;
        // DEPRECATED wil be removed
        Apollo::RegionContext *current_context;
        //
        std::ofstream trace_file;

        std::vector<Apollo::RegionContext *> pending_contexts;
        void collectContext(Apollo::RegionContext *, double);
}; // end: Apollo::Region

struct Apollo::RegionContext
{
    double exec_time_begin;
    double exec_time_end;
    std::vector<float> features;
    int policy;
    int idx;
    // Arguments: void *data, bool *returnMetric, double *metric (valid if
    // returnsMetric == true).
    bool (*isDoneCallback)(void *, bool *, double *);
    void *callback_arg;
}; //end: Apollo::RegionContext

struct Apollo::CallbackDataPool {
    virtual ~CallbackDataPool() = default;
    virtual void put(void *) = 0;
    virtual void *get() = 0;
};

#endif
