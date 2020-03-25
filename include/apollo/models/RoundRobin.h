#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <string>

#include "apollo/Model.h"

class RoundRobin : public Model {
    public:
        RoundRobin(int num_policies);
        ~RoundRobin();

        int  getIndex(std::vector<float> &features);

    private:
        static int policy_choice;

}; //end: RoundRobin (class)


#endif
