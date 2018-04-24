#ifndef APOLLO_H
#define APOLLO_H

#include "sos.h"
#include "sos_types.h"


#define APOLLO_VERBOSE 1

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
        

        
        
        
        
        
        
        //Function ideas:
        //  setLearnMode()
        //  getLearnMode()
        //  double getHysterisis()
        //  requestDecisionTree()
        //  generateMultiPolicy()
        //  
        // 

        //NOTE: Eventually, let's disallow certain operators.
        //Apollo(const Apollo&) = delete;
        //Apollo& operator=(const Apollo&) = delete;
        
    private:
        SOS_runtime     *sos;
        SOS_pub         *pub;
};

#endif
