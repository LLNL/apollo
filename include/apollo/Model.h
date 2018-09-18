#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include "apollo/Apollo.h"

// Forward declaration of the group of models, each of which will be
// inheriting from ModelClass (below)...
class Apollo::Model {
    class Random;
    class Sequential;
    class DecisionTree;
    class Python;
};

class Apollo::ModelClass {
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

}; //end: Apollo::Model (class)


#endif
