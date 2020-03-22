#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include <string>

#include "apollo/Apollo.h"

#define APOLLO_DEFAULT_MODEL_TYPE   Apollo::Model::Type::Random

// Abstract
class Apollo::Model {
    public:
        // Forward declarations of model types:
        class Random       ; // : public ModelObject;
        class Sequential   ; // : public ModelObject;
        class Static       ; // : public ModelObject;
        class RoundRobin   ; // : public ModelObject;
        class DecisionTree ; // : public ModelObject;

        // pure virtual function (establishes this as abstract class)
        virtual void configure(int num_policies) = 0;
        //
        virtual int      getIndex(void) = 0;

        void     setGuid(uint64_t ng) { guid = ng; return;}
        uint64_t getGuid(void)        { return guid; }

        std::string      name           = "";
        bool             training       = false;

    protected:
        Apollo      *apollo;
        //
        bool         configured = false;
        //
        uint64_t     guid;
        int          policy_count;
}; //end: Apollo::ModelObject (abstract class)


#endif
