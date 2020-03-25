#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Sequential : public Model {
    public:
        Sequential();
        ~Sequential();

        void configure(int num_policies);
        //
        bool learning = true;
        int  getIndex(std::vector<float> &features);

    private:

}; //end: Sequential (class)


#endif
