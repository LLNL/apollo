
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <unordered_map>

#include "apollo/Apollo.h"

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

        int                   getPolicyIndex(void);

        std::unordered_map<std::vector<Apollo::Feature>, Apollo::Region::Measure *>
            measures;

        int            current_policy;

        void packMeasurements(char *buf, int size, MPI_Comm comm);
        // TODO: add model object for region
        Apollo::Model *model;

    private:
        //
        Apollo        *apollo;
        bool           currently_inside_region;
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
