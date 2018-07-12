
#include <string.h>

#include "apollo/Apollo.h"
#include "apollo/Feature.h"

Apollo::Feature::Feature(
        Apollo *apolloContext,
        const char *featureName,
        Apollo::Hint hintVal,
        Apollo::Goal goalVal,
        Apollo::Unit unitVal,
        void *ptrToVal)
{
    apollo = apolloContext;
    name = strdup(featureName);
    hint = hintVal;
    goal = goalVal;
    unit = unitVal;
    objPtr  = ptrToVal;
    return;
}

Apollo::Feature::~Feature()
{
    free(name);
    apollo->features.remove(this);
}

const char *Apollo::Feature::getName(void)   { return name; }
int         Apollo::Feature::getHint(void)   { return to_underlying(hint); }
int         Apollo::Feature::getGoal(void)   { return to_underlying(goal); }
int         Apollo::Feature::getUnit(void)   { return to_underlying(unit); }
void       *Apollo::Feature::getObjPtr(void) { return objPtr; }


