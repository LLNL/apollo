#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::RoundRobin : public Apollo::ModelObject {
    public:
        RoundRobin();
        ~RoundRobin();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                std::string model_definition);
        //
        int  getIndex(void);

    private:
        int policy_choice;

}; //end: Apollo::Model::RoundRobin (class)


#endif
