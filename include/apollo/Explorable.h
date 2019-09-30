
#ifndef APOLLO_EXPLORABLE_H
#define APOLLO_EXPLORABLE_H

#include <string>
#include <vector>

#include "apollo/Apollo.h"


class Apollo::Explorable {
    public:
        Explorable( std::string          name,
                    double              *target_var,
                    std::vector<int>     for_policies, 
                    std::vector<double>  values);
        ~Explorable() {};
       
        std::string              name;
        double                  *target_var;
        std::vector<double>      values;

        int                      offset;

        bool applyNextValueLooped(void);
}; //end: Apollo::Explorable


#endif
