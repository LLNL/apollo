#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include "apollo/Apollo.h"

#define APOLLO_DEFAULT_MODEL_TYPE   Apollo::Model::Type::Random

class Apollo::Model {
    public:
        // Forward declarations of model types.
        class Random       ; // : public ModelObject;
        class Sequential   ; // : public ModelObject;
        class DecisionTree ; // : public ModelObject;
        class Python       ; // : public ModelObject;

        class Type {
            public:
                static constexpr int Default      = 0;
                static constexpr int Random       = 1;
                static constexpr int Sequential   = 2;
                static constexpr int DecisionTree = 3;
                static constexpr int Python       = 4;
        };
};

// Abstract
class Apollo::ModelObject {
    public:
        // pure virtual function (establishes this as abstract class)
        virtual void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition) = 0;
        //
        virtual int  getIndex(void) = 0;

    protected:
        Apollo      *apollo;
        //
        bool         configured = false;
        //
        uint64_t     id;
        int          policyCount;
        std::string  model_def;
        int          iterCount;

}; //end: Apollo::ModelObject (abstract class)


#endif
