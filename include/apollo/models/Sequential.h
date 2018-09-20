#ifndef APOLLO_MODELS_SEQUENTIAL_H
#define APOLLO_MODELS_SEQUENTIAL_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Sequential : public Apollo::ModelObject {
    public:
        Model();
        virtual ~Model();

        virtual void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition);
        //
        virtual int  getIndex(void);

    private:

}; //end: Apollo::Model::Sequential (class)


#endif
