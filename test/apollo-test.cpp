
#include <iostream>
#include <chrono>

#include "apollo/Apollo.h"
#include "apollo/Region.h"
#include "apollo-test.h"
#include "mpi.h"


#define NUM_FEATURES 1
#define NUM_POLICIES 4
#define FLUSH        (NUM_POLICIES * NUM_POLICIES)
#define ITERS        (2 * FLUSH)
#define DELAY        1

int main()
{
    MPI_Init(NULL, NULL);
    int rc = 0;
    int match = 0;
    fprintf(stdout, "testing Apollo.\n");

    Apollo *apollo = Apollo::instance();

    Apollo::Region *r = new Apollo::Region(NUM_FEATURES, "test-region1", NUM_POLICIES);

    for (int i = 0; i < ITERS; i++)
    {
        int feature = i % NUM_POLICIES;
        r->begin();
        r->setFeature(float(feature));

        int policy = r->getPolicyIndex();

        printf("Feature %d Policy %d\n", feature, policy);

        if(policy != feature) {
            auto start = std::chrono::steady_clock::now();
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            while(elapsed.count() < DELAY) {
                end = std::chrono::steady_clock::now();
                elapsed = end - start;
            }
        }
        else {
            printf("Match!\n");
            match++;
        }

        if( ( i>0 && i%FLUSH) == 0 ) {
            printf("Install model i %d FLUSH %d\n", i, FLUSH);
            apollo->flushAllRegionMeasurements(i);
        }

        r->end();
    }

    printf("matched region 1 %d / %d\n", match, ITERS);

    match = 0;

    r = new Apollo::Region(NUM_FEATURES, "test-region2", NUM_POLICIES);

    for (int i = 0; i < ITERS; i++)
    {
        int feature = i % NUM_POLICIES;
        r->begin();
        r->setFeature(float(feature));

        int policy = r->getPolicyIndex();

        printf("Feature %d Policy %d\n", feature, policy);

        if(policy != feature) {
            auto start = std::chrono::steady_clock::now();
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            while(elapsed.count() < DELAY) {
                end = std::chrono::steady_clock::now();
                elapsed = end - start;
            }
        }
        else {
            printf("Match!\n");
            match++;
        }

        if( ( i>0 && i%FLUSH) == 0 ) {
            printf("Install model i %d FLUSH %d\n", i, FLUSH);
            apollo->flushAllRegionMeasurements(i);
        }

        r->end();
    }

    printf("matched region 2 %d / %d\n", match, ITERS);
    fprintf(stdout, "testing complete.\n");

    MPI_Finalize();

    return rc;
}
