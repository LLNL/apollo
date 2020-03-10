#ifndef APOLLO_H
#define APOLLO_H

#include <cstdint>
#include <string>
#include <mutex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <type_traits>
#include <list>
#include <vector>

#include <omp.h>

#include "apollo/Logging.h"

#include "CallpathRuntime.h" //ggout

#define APOLLO_DEFAULT_MODEL_CLASS          Apollo::Model::Static

extern "C" {
    // SOS will deliver triggers and query results here:
    void
    handleFeedback(void *sos_context,
                   int msg_type,
                   int msg_size,
                   void *data);

    // Used to call Apollo functions from the above C code:
    void call_Apollo_attachModel(void *apollo, const char *def);
}



class Apollo
{
    public:
       ~Apollo();
        // disallow copy constructor
        Apollo(const Apollo&) = delete;
        Apollo& operator=(const Apollo&) = delete;

        static Apollo* instance(void) noexcept {
            static Apollo the_instance;
            return &the_instance;
        }

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
        class Model;
        class ModelObject;
        class ModelWrapper;
        class Explorable;

        class Feature
		{
			public:
	            std::string    name;
    	        double         value;
				//
				bool operator == (const Feature &other) const {
					if(name == other.name) { return true; } else { return false; }
				}
		};
        //
        std::vector<Apollo::Feature>            features;
        //
        // Precalculated at Apollo::Init from evironment variable strings to
        // facilitate quick calculations during model evaluation later.
        int numNodes;
        int numCPUsOnNode;
        int numProcs;
        int numProcsPerNode;
        int numThreadsPerProcCap;
        omp_sched_t ompDefaultSchedule;
        int         ompDefaultNumThreads;
        int         ompDefaultChunkSize;
        //
        int numThreads;  // <-- how many to use / are in use
        //
        void    setFeature(std::string ft_name, double ft_val);
        double  getFeature(std::string ft_name);

        // NOTE(chad): We default to walk_distance of 2 so we can
        //             step out of this method, then step out of
        //             our RAJA policy template, and get to the
        //             module name and offset where that template
        //             has been instantiated in the application code.
        std::string getCallpathOffset(int walk_distance=2);
        void *callpath_ptr;

        Apollo::Region *region(const char *regionName);
        //
        std::string model_def = ""; //ggout
        void attachModel(const char *modelEncoding);
        //
        bool isOnline();
        std::string uniqueRankIDText(void);
        //
        void flushAllRegionMeasurements(int assign_to_step);
        //
        void *sos_handle;  // #include "sos_types.h":  SOS_runtime *sos = sos_handle;
        void *pub_handle;  // #include "sos_types.h":  SOS_pub     *pub = pub_handle;
        //
        // Utility functions for SOS/environment interactions:
        int  sosPackInt(const char *name, int val);
        int  sosPackDouble(const char *name, double val);
        int  sosPackRelatedInt(uint64_t relation_id, const char *name, int val);
        int  sosPackRelatedDouble(uint64_t relation_id, const char *name, double val);
        int  sosPackRelatedString(uint64_t relation_id, const char *name, const char *val);
        int  sosPackRelatedString(uint64_t relation_id, const char *name, std::string val);
        void sosPublish();
        //
        void disconnect();
        //
    private:
        Apollo();
        Apollo::Region *baseRegion;
        std::mutex regions_lock;
        std::map<const char *, Apollo::Region *> regions;
        std::list<Apollo::ModelWrapper *> models;

        void *getContextHandle();
        bool  ynConnectedToSOS;

}; //end: Apollo

// Implemented in src/Region.cpp:
extern int
getApolloPolicyChoice(Apollo::Region *reg);

inline const char*
safe_getenv(
        const char *var_name,
        const char *use_this_if_not_found,
        bool        silent=false)
{
    char *c = getenv(var_name);
    if (c == NULL) {
        if (not silent) {
            log("Looked for ", var_name, " with getenv(), found nothing, using '", \
                use_this_if_not_found, "' (default) instead.");
        }
        return use_this_if_not_found;
    } else {
        return c;
    }
}

template <class T>
inline void hash_combine(std::size_t& seed, T const& v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
    template<>
    struct hash<vector<Apollo::Feature> >
    {
        typedef vector<Apollo::Feature> argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& in) const
        {
            size_t size = in.size();
            size_t seed = 0;
            for (size_t i = 0; i < size; i++) {
                //Combine hash of current vector with hashes of previous ones
                hash_combine(seed, in[i].name);
                hash_combine(seed, in[i].value);
			}
            return seed;
        }
    };
}


template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}



#endif


