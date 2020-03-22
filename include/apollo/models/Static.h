#ifndef APOLLO_MODELS_STATIC_H
#define APOLLO_MODELS_STATIC_H

#include <string>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Static : public Apollo::Model {
    public:
        Static();
        ~Static();

        void configure(int num_policies);
        //
        int  getIndex(void);

    private:
        int policy_choice;

}; //end: Apollo::Model::Static (class)


#endif
