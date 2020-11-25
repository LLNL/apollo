
// Copyright (c) 2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


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
