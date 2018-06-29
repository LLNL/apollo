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

        class Model {
            public:
                Model(Apollo *apollo, const char *id, int numAvailablePolicies);
                ~Model();
               
                int  requestPolicyIndex(void);

            private:
                Apollo *apollo;
                char   *model_id;
                char   *model_pattern;   // TODO: This will evolve.
                //
                int     current_policy_index;
        };

        class Region {
            public:
                Region(Apollo *apollo,
                        const char *regionName,
                        int numAvailablePolicies);
                ~Region();
                // disallow copy constructor
                Region(const Region&) = delete; 
                Region& operator=(const Region&) = delete;
                // 
                void begin(void); //Default begin(): f.hint != Feature::Hint.DEPENDANT
                void begin(std::list<Apollo::Feature *> logOnlyTheseSpecificFeatures);
                void begin(std::list<Apollo::Feature::Hint> logAnyMatchingTheseHints);
                //
                void iterationStart(int i);
                void iterationStop(void);
                //
                void end(void);   //Default end(): f.hint == Feature::Hint.DEPENDANT
                void end(std::list<Apollo::Feature *> logOnlyTheseSpecificFeatures);
                void end(std::list<Apollo::Feature::Hint> logAnyMatchingTheseHints);
                //
                Apollo::Model *getModel(void);
                //
                std::list<Apollo::Feature *> getFeatures(void);
                std::list<Apollo::Feature *> getFeatures(
                        Apollo::Feature::Hint getAllMatchingThisHint);
                //
                Apollo::Feature *defineFeature(
                        const char *featureName,
                        Apollo::Feature::Hint hint,
                        Apollo::Feature::Goal goal,
                        Apollo::Feature::Unit unit,
                        void *ptr_to_var); //NOTE: Safe to call multiple times.
                //                                 (re-call will update `ptr_to_var`)
                //
                                
            private:
                void handleCommonBeginTasks(void);
                void handleCommonEndTasks(void);
                void packFeature(Apollo::Feature *feature);
                void caliSetInt(const char *name, int value);
                void caliSetString(const char *name, const char *value);

                //
                Apollo        *apollo;
                char          *name;
                Apollo::Model *model;
                uint64_t       id;
                uint64_t       parent_id;
                bool           ynInsideMarkedRegion;
                char           CURRENT_BINDING_GUID[256];
                cali::Loop            *cali_obj;
                cali::Loop::Iteration *cali_iter_obj;
        }; //end: Apollo::Region

        class Feature {
            public:
                enum class Hint : int {
                    MEMO,
                    INDEPENDENT,
                    DEPENDANT
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

                
                Feature(Apollo::Region *region_ptr,
                        const char *featureName,
                        Apollo::Feature::Unit unit,
                        Apollo::Feature::Hint hint,
                        Apollo::Feature::Goal goal,
                        void *ptr_to_var);
                ~Feature();
            private:
                Apollo::Region *region_ptr;
                //
                char *name;
                Apollo::Feature::Hint hint;
                Apollo::Feature::Goal goal;
                Apollo::Feature::Unit unit;
                void *obj_ptr;
        };

        // NOTE: This will apply to all Caliper data not
        //       generated in the context of an Apollo::Region
        char GLOBAL_BINDING_GUID[256];

        Apollo::Region *region(const char *regionName); 

        bool connect();
        bool isOnline();
        //
        void disconnect();

    private:
        std::map<const char *, Apollo::Region *> regions;
        void *getContextHandle();
        bool  ynConnectedToSOS;
        

};

    
RAJA_INLINE
int getApolloPolicyChoice(Apollo::Region *loop) 
{
    int choice = 0;

    if (loop != NULL) {
        Apollo::Model *model = loop->getModel();
        if (model != NULL) {
            choice = model->requestPolicyIndex();
        }
    }

    loop->setNamedInt("policyIndex", choice);

    return choice;
}


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


