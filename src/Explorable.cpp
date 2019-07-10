
#include <vector>
#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/Explorable.h"


    // NOTE: If we want to grab a GUID for some future annotation, this is how:
    // ----
    //SOS_guid guid = 0;
    //if (apollo->isOnline()) {
    //    guid = SOS_uid_next(sos->uid.my_guid_pool);
    //}

Apollo::Explorable::Explorable(
        std::string          set_name,
        double              *set_target_var,
        std::vector<int>     set_for_policies,
        std::vector<double>  set_values)
{
    assert (set_target_var != NULL);
    assert (set_target_var != nullptr);
    assert (set_for_policies.size() > 0);
    assert (set_values.size() > 0);

    name         = set_name;
    target_var   = set_target_var;

    offset = 0;
    *target_var  = values[0];

    return;
}



