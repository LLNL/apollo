#ifndef APOLLO_MODELS_PYTHON_H
#define APOLLO_MODELS_PYTHON_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::Python : public Apollo::ModelObject {
    public:
        Python();
        ~Python();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition);
        //
        int  getIndex(void);

    private:

}; //end: Apollo::Model::Python (class)


#endif
