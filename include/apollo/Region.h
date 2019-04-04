
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"

class Apollo::Region {
    public:
        Region( Apollo       *apollo,
                const char   *regionName,
                int           numAvailablePolicies);
        ~Region();

        // NOTE: Feature is not in use currently, but
        //       is a concept that needs to get revisited in
        //       future updates to Apollo, along with
        //       Explorables.
        // ----
        class Feature;
        class Explorable;
        //
        // NOTE: the parameter to begin() should be the time step
        //       of the broader experimental context that this
        //       region is being invoked to service. It may get
        //       invoked several times for that time step, that is fine,
        //       those counts are automatically tracked internally.
        //       If you do not know or have access to that time step,
        //       passing in (and incrementing) a static int from the
        //       calling context is typical.
        void begin(int for_experiment_time_step);
        void end(void);
        //
        Apollo::ModelWrapper *getModel(void);
        int                   getPolicyIndex(void);
        //
        void caliSetInt(const char *name, int value);
        void caliSetString(const char *name, const char *value);
        //
        char          *name;
 
    private:
        //
        Apollo        *apollo;
        bool           currently_inside_region;
        //
        Apollo::ModelWrapper  *model;
        //
        int            current_step;
        int            current_policy;
        int            exec_count_total;
        int            exec_count_current_step;
        int            exec_count_current_policy;
        //
        // cali::Annotation *
        void          *note_region_name;
        void          *note_current_step;
        void          *note_current_policy;
        void          *note_exec_count_total;
        void          *note_exec_count_current_step;
        void          *note_exec_count_current_policy;

        //
        // Deprecated (somewhat, look into for cleanup):
        //void          *loop_obj;                 // cali::Loop *
        uint64_t       id;
        uint64_t       parent_id;
        char           CURRENT_BINDING_GUID[256];
}; //end: Apollo::Region


#endif
