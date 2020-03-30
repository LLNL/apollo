#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <string>

#include "apollo/PolicyModel.h"

class RoundRobin : public PolicyModel {
    public:
        RoundRobin(int num_policies);
        ~RoundRobin();

        int  getIndex(std::vector<float> &features);

    private:
        int policy_choice;
        int offset;

}; //end: RoundRobin (class)


#endif
