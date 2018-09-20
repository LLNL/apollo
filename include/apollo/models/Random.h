#ifndef APOLLO_MODELS_RANDOM_H
#define APOLLO_MODELS_RANDOM_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Random : public Apollo::ModelObject {
    public:
        Random();
        virtual ~Random();

        virtual void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition);
        //
        virtual int  getIndex(void);

    private:

}; //end: Apollo::Model::Random (class)


#endif
