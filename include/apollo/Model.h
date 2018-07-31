#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include "apollo/Apollo.h"

// NOTE: This is the header used to define actual models that
//       can be built and "plugged in" to Apollo at runtime.
//       These concrete models are managed through an interface
//       provided by the Apollo::ModelWrapper class.

class Apollo::Model {
    public:
        Model();
        ~Model();

        virtual void configure(Apollo *apollo_ptr);
        virtual int  getIndex(void);

    private:
        Apollo    *apollo;
        uint64_t   id;
        int        policyCount;
        int        iterCount;
        

}; //end: Apollo::Model (class)


#endif
