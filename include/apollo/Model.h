#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include "apollo/Apollo.h"

#define APOLLO_DEFAULT_MODEL_TYPE   Apollo::Model::Type::Random

class Apollo::Model {
    // Forward declarations of modely types.
    class Random;
    class Sequential;
    class DecisionTree;
    class Python;

    // This is ugly, but *something* is needed to coordinate w/Python
    enum class Type : int {
        Default      = 0,
        Random       = 1,
        Sequential   = 2,
        DecisionTree = 3,
        Python       = 4
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
