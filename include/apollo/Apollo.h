#ifndef APOLLO_H
#define APOLLO_H

#include <cstdint>
#include <string>
#include <mutex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <type_traits>
#include <list>
#include <vector>

#include "apollo/Logging.h"

#include "CallpathRuntime.h"

#define APOLLO_DEFAULT_MODEL_CLASS          Apollo::Model::Static
#define APOLLO_DEFAULT_MODEL_DEFINITION     "0"


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

        static Apollo* instance(void) {
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
        std::unordered_map<std::string, void *> feature_notes; // cali::Annotation *
        //
        // Precalculated at Apollo::Init from evironment variable strings to
        // facilitate quick calculations during model evaluation later.
        int numNodes;
        int numCPUsOnNode;
        int numProcs;
        int numProcsPerNode;
        int numThreadsPerProcCap;
        //
        void *  getNote(std::string &name);
        void    noteBegin(std::string &name, double with_value);
        void    noteEnd(std::string &name);
        //
        void    setFeature(std::string ft_name, double ft_val);
        double  getFeature(std::string ft_name);

        CallpathRuntime callpath;

        Apollo::Region *region(const char *regionName);
        //
        void attachModel(const char *modelEncoding);
        //
        bool        isOnline();
        std::string uniqueRankIDText(void);
        //
        void flushAllRegionMeasurements(int assign_to_step);
        //
        int  sosPack(const char *name, int val);
        int  sosPackRelated(long relation_id, const char *name, int val);
        void sosPublish();
        //
        void disconnect();

    private:
        Apollo();
        Apollo::Region *baseRegion;
        std::map<const char *, Apollo::Region *> regions;
        std::list<Apollo::ModelWrapper *> models;

        void *getContextHandle();
        bool  ynConnectedToSOS;

}; //end: Apollo

// Implemented in src/Region.cpp:
extern int
getApolloPolicyChoice(Apollo::Region *reg);


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


