
#ifndef APOLLO_FEATURE_H
#define APOLLO_FEATURE_H

#include "apollo/Apollo.h"

class Apollo::Feature {
    public:

        Feature(Apollo *apolloContext,
                const char *fetureName,
                Apollo::Hint hint,
                Apollo::Goal goal,
                Apollo::Unit unit,
                void *ptr_to_var);
        ~Feature();

        const char *getName(void);
        int         getHint(void);
        int         getGoal(void);
        int         getUnit(void);
        void       *getObjPtr(void);

        void pack(void);

    private:
        Apollo        *apollo;
        char          *name;
        Apollo::Hint   hint;
        Apollo::Goal   goal;
        Apollo::Unit   unit;
        void          *objPtr;
}; // end: Apollo::Feature



#endif

