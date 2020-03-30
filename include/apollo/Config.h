#ifndef APOLLO_CONFIG_H
#define APOLLO_CONFIG_H

class Config {
    public:
        static int APOLLO_COLLECTIVE_TRAINING;
        static int APOLLO_LOCAL_TRAINING;
        static int APOLLO_GLOBAL_MODEL;
        static int APOLLO_REGION_MODEL;
        static std::string APOLLO_INIT_MODEL;

    private:
        Config();
};

#endif
