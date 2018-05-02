#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <string>

class Apollo::Region
{
    public:
        Apollo::Region();
        ~Apollo::Region();
        
        Apollo(const Apollo&) = delete; // disallow copy constructor
        Apollo& operator=(const Apollo&) = delete;

        friend class Apollo::Region;

        bool isOnline();

    private:
        void *getContextHandle();
        bool  ynConnectedToSOS;
        PublishUpdates(Apollo::Region *rgn);
};

#endif


