#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Sequential : public Apollo::Model {
    public:
        Sequential();
        ~Sequential();

        void configure(int num_policies);
        //
        bool learning = true;
        int  getIndex(void);

    private:

}; //end: Apollo::Model::Sequential (class)


#endif
