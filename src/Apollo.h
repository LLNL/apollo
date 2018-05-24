#ifndef APOLLO_H
#define APOLLO_H

#include <string>

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
        
        Apollo(const Apollo&) = delete; // disallow copy constructor
        Apollo& operator=(const Apollo&) = delete;

        bool isOnline();

    private:
        void *getContextHandle();
        bool  ynConnectedToSOS;
        void PublishUpdates(void *rgn);
};

#endif


