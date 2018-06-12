#ifndef APOLLO_H
#define APOLLO_H

#include <cstdint>
#include <string>
#include <mutex>
#include <map>

#include "caliper/cali.h"
#include "caliper/Annotation.h"

#ifndef APOLLO_VERBOSE
#define APOLLO_VERBOSE 1
#endif

#if (APOLLO_VERBOSE < 1)
    // Nullify the variadic macro for production runs.
    #define apollo_log(level, ...)
#else
    #define apollo_log(level, ...)                                      \
    {   if (level <= APOLLO_VERBOSE) {                                  \
            fprintf(stdout, "APOLLO: ");                                \
            fprintf(stdout, __VA_ARGS__);                               \
            fflush(stdout);                                             \
    }   };
#endif

extern "C" {
    // SOS will delivery triggers and query results here:
    void  
    handleFeedback(int msg_type,
                   int msg_size,
                   void *data);
}


class Apollo
{
    public:
        Apollo();
        ~Apollo();
        // disallow copy constructor 
        Apollo(const Apollo&) = delete; 
        Apollo& operator=(const Apollo&) = delete;

        enum class DataType { INT, DOUBLE, STRING, BYTES };

        class Model {
            public:
                Model(Apollo *apollo, const char *id);
                ~Model();
               
                int  requestPolicyIndex(void);

            private:
                char *model_obj;   // TODO: This will evolve.

        };

        class Region {
            public:
                Region(Apollo *apollo, const char *name);
                ~Region();
                // disallow copy constructor
                Region(const Region&) = delete; 
                Region& operator=(const Region&) = delete;
                
                void begin(void);
                void iterationStart(int i);
                void iterationStop(void);
                void end(void);
                //
                // NOTE: Eventually named_int will get moved into setFeature()
                //
                void setNamedInt(const char *name, int value);
                //
                void setFeature(
                        const char *name,
                        Apollo::DataType featType,
                        void *value);
 
            private:
                Apollo        *apollo;
                char          *name;
                Apollo::Model *model;
                uint64_t       id;
                uint64_t       parent_id;
                //
                bool           ynInsideMarkedRegion;
                //
                cali::Loop            *cali_obj;
                cali::Loop::Iteration *cali_iter_obj;
        };

        bool connect();
        bool isOnline();
        //
        void disconnect();

    private:
        void *getContextHandle();
        bool  ynConnectedToSOS;
        

};

    
RAJA_INLINE
int getApolloPolicyChoice(Apollo::Region *loop) 
{
    int choice = 0;

    if ((loop != NULL)
     && (loop->model_obj != NULL)) {
        choice = loop->model_obj->requestPolicyIndex();
    }

    return choice;
}


#endif


