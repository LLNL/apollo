
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <unordered_map>

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Explorable.h"

#include <mpi.h>

class Apollo::Region {
    public:
        Region(
                const char   *regionName,
                int           numAvailablePolicies);
        ~Region();

        typedef struct {
            int       exec_count;
            double    time_total;
        } Measure;


        char    name[64];

        void     begin();
        void     end();

        Apollo::ModelWrapper *getModelWrapper(void);
        int                   getPolicyIndex(void);

        std::unordered_map<std::vector<Apollo::Feature>, Apollo::Region::Measure *>
            measures;

        int            current_policy;

        void           flushMeasurements(int assign_to_step);
        
        void packMeasurements(char *buf, int size, MPI_Comm comm);

    private:
        //
        Apollo        *apollo;
        bool           currently_inside_region;
        //
        Apollo::ModelWrapper *model_wrapper;
        //
        double         current_exec_time_begin;
        double         current_exec_time_end;
        //
        uint64_t       id;
        uint64_t       parent_id;
        //
        //
}; //end: Apollo::Region


#endif
