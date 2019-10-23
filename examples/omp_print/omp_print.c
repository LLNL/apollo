#define _GNU_SOURCE
#include <stdio.h>
#include <omp.h>
#include <pthread.h>
#include <sched.h>
#define NCPUS 36
int main()
{
    char aff[36][128];
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        printf("Hello from thread %d\n", tid );
        cpu_set_t set;
        pthread_getaffinity_np( pthread_self(), sizeof(cpu_set_t), &set);
        char *p = aff[tid];
        int j;
        for(j=0; j<NCPUS;j++)
            p += sprintf(p, "%2d ", CPU_ISSET( j, &set) ? 1 : 0 );
    }
    int j;
    printf("CORE ");
    for(j=0; j<NCPUS;j++)
        printf("%02d ", j);
    printf("\n");
    for(j=0; j<NCPUS; j++) {
        printf("T%2d: %s\n", j, aff[j]);
    }
    return 0;
}
