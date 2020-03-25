#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Static : public Model {
    public:
        Static(int num_policies, int policy_choice) : 
           Model(num_policies, "Static", false), policy_choice(policy_choice) {};
        ~Static() {};

        //
        int  getIndex(std::vector<float> &features);

    private:
        int policy_choice;

}; //end: Static (class)


#endif
