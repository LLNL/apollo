
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <chrono>
#include <memory>
#include <map>

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
                int           numAvailablePolicies);
        ~Region();

        typedef struct Measure {
            int       exec_count;
            double    time_total;
            Measure(int e, double t) : exec_count(e), time_total(t) {}
        } Measure;

        char     name[64];

        void     begin();
        void     end();
        void     end(double synthetic_duration_or_weight);
                    // lower == better, 0.0 == perfect

        int      getPolicyIndex(void);

        int      current_policy;
        int      current_elem_count;

        int      num_features;
        void     setFeature(float value);
        int      reduceBestPolicies(int step);

        //NOTE(cdw): Moving this to Apollo class for MPI-OpenCV to access MPI_comm obj.
        //void     packMeasurements(char *buf, int size);

        std::map<
            std::vector< float >,
            std::pair< int, double > > best_policies;

        std::map<
            std::pair< std::vector<float>, int >,
            std::unique_ptr<Apollo::Region::Measure> > measures;
        //^--Explanation: < features, policy >, value: < time measurement >

        std::unique_ptr<TimingModel> time_model;
        std::unique_ptr<PolicyModel> model;

    private:
        //
        Apollo        *apollo;
        bool           currently_inside_region;
        //
        std::chrono::steady_clock::time_point current_exec_time_begin;
        std::chrono::steady_clock::time_point current_exec_time_end;
        std::vector<float>            features;
        //
}; //end: Apollo::Region

#endif
