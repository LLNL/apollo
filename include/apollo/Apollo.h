#ifndef APOLLO_H
#define APOLLO_H

#include <cstdint>
#include <string>
#include <mutex>
#include <map>
#include <type_traits>

#include "caliper/cali.h"
#include "caliper/Annotation.h"
//
#include "RAJA/RAJA.hpp"

 
#define APOLLO_DEFAULT_MODEL_CLASS          Apollo::Model::Random
#define APOLLO_DEFAULT_MODEL_DEFINITION     "N/A (Random)"

#ifndef APOLLO_VERBOSE
#define APOLLO_VERBOSE 0 
#endif

#if (APOLLO_VERBOSE < 0)
    // Nullify the variadic macro for production runs.
    #define apollo_log(level, ...)
#else
    #define apollo_log(level, ...)                                      \
    {   if (level <= APOLLO_VERBOSE) {                                  \
            fprintf(stdout, "== APOLLO: ");                             \
            fprintf(stdout, __VA_ARGS__);                               \
            fflush(stdout);                                             \
    }   };
#endif

extern "C" {
    // SOS will deliver triggers and query results here:
    void  
    handleFeedback(void *sos_context,
                   int msg_type,
                   int msg_size,
                   void *data);

    // Used to call Apollo functions from the C code:
    void call_Apollo_attachModel(void *apollo, const char *def);
}

class Apollo
{
    public:
        Apollo();
        ~Apollo();
        // disallow copy constructor 
        Apollo(const Apollo&) = delete; 
        Apollo& operator=(const Apollo&) = delete;

        enum class Hint : int {
            INDEPENDENT,
            DEPENDENT
        };

        enum class Goal : int {
            OBSERVE,
            MINIMIZE,
            MAXIMIZE
        };

        enum class Unit : int {
            INTEGER,
            DOUBLE,
            CSTRING 
        };


        // Forward declarations:
        class Region;
        class Feature;
        class Model;
        class ModelObject;
        class ModelWrapper;
        //...

        Apollo::Region *region(const char *regionName);

        std::list<Apollo::Feature *> getFeatures(void);
        std::list<Apollo::Feature *> getFeatures(
                Apollo::Hint getAllMatchingThisHint);
        //
        Apollo::Feature defineFeature(
                const char *featureName,
                Apollo::Goal goal,
                Apollo::Unit unit,
                void *ptr_to_var); 
        //
        void defineAdvancedFeature(const char *featureName,
                Apollo::Hint hint,
                Apollo::Goal goal,
                Apollo::Unit unit,
                void *ptr_to_var); //NOTE: Safe to call multiple times.
        //                                 (re-call will update `ptr_to_var`)
        //
        void attachModel(const char *modelEncoding);
        // 
        bool isOnline();
        //
        void publish();
        //
        void disconnect();

    private:
        Apollo::Region *baseRegion;
        std::map<const char *, Apollo::Region *> regions;
        std::list<Apollo::ModelWrapper *> models;
        std::list<Apollo::Feature *> features;
        //
        void *getContextHandle();
        bool  ynConnectedToSOS;

}; //end: Apollo

    

// C++14 version of template that converts enum classes into
// their underlying type, i.e. an int:
//template <typename E>
//constexpr auto to_underlying(E e) noexcept
//{
//    return static_cast<std::underlying_type_t<E>>(e);
//}

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}



#endif


