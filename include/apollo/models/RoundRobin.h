#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::RoundRobin : public Apollo::Model {
    public:
        RoundRobin();
        ~RoundRobin();

        void configure(int num_policies);
        //
        int  getIndex(void);

    private:
        int policy_choice;

}; //end: Apollo::Model::RoundRobin (class)


#endif
