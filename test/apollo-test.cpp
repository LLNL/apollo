#include <iostream>

#include "apollo/Apollo.h"
#include "apollo-test.h"

int main()
{
    int rc = 0;
    fprintf(stdout, "testing Apollo.\n");

    Apollo *apollo = Apollo::instance();
    


    fprintf(stdout, "testing complete.\n");

    return rc;
}
