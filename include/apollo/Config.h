#ifndef APOLLO_CONFIG_H
#define APOLLO_CONFIG_H

// TODO: create a better configuration method
#define APOLLO_COLLECTIVE_TRAINING 0

#define APOLLO_GLOBAL_MODEL 1
#if APOLLO_GLOBAL_MODEL
#define APOLLO_REGION_MODEL 0
#else
#define APOLLO_REGION_MODEL 1
#endif

#endif
