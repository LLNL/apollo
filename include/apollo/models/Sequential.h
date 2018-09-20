#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Sequential : public Apollo::ModelObject {
    public:
        Sequential();
        ~Sequential();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition);
        //
        int  getIndex(void);

    private:

}; //end: Apollo::Model::Sequential (class)


#endif
