
#include <string.h>

#include "sos.h"
#include "sos_types.h"

#include "apollo/Apollo.h"
#include "apollo/Feature.h"

extern SOS_runtime     *sos;
extern SOS_pub         *pub;

/* EXAMPLE:    (deprecated, use Caliper directly)
 *
 * auto vecSize =
        apollo->defineFeature(
            "vector_size",
            Apollo::Goal::OBSERVE,
            Apollo::Unit::INTEGER,
            (void *) &N);
 */

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

    apollo->features.push_back(this);
    
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

void
Apollo::Feature::pack(void)
{
    SOS_val_type sosType;
    switch (unit) {
        case Apollo::Unit::INTEGER: sosType = SOS_VAL_TYPE_INT;    break;
        case Apollo::Unit::DOUBLE:  sosType = SOS_VAL_TYPE_DOUBLE; break;
        case Apollo::Unit::CSTRING: sosType = SOS_VAL_TYPE_STRING; break;
    default:                        sosType = SOS_VAL_TYPE_INT;    break;
    }

    SOS_guid groupID = SOS_uid_next(sos->uid.my_guid_pool);

	int ft = to_underlying(Apollo::Hint::DEPENDENT);
	int op = to_underlying(goal);
	int du = to_underlying(unit);
   
    SOS_pack_related(pub, groupID, "featureName", SOS_VAL_TYPE_STRING, name);
	SOS_pack_related(pub, groupID, "featureHint", SOS_VAL_TYPE_INT, &ft);
	SOS_pack_related(pub, groupID, "featureGoal", SOS_VAL_TYPE_INT, &op);
	SOS_pack_related(pub, groupID, "featureUnit", SOS_VAL_TYPE_INT, &du);
	SOS_pack_related(pub, groupID, "featureValue", sosType, objPtr);

	// Features are only set from outside of loops,
	// publishing each time is not a performance hit
	// in the client.
	// SOS_publish(pub);

    return;
}
