#ifndef APOLLO_MODELS_ROUNDROBIN_H
#define APOLLO_MODELS_ROUNDROBIN_H

#include <string>
#include <vector>
#include <map>

#include "apollo/PolicyModel.h"

class RoundRobin : public PolicyModel {
    public:
        RoundRobin(int num_policies);
        ~RoundRobin();

        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename) {};

    private:
        std::map< std::vector<float>, int > policies;

}; //end: RoundRobin (class)


#endif
