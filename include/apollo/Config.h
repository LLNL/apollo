#ifndef APOLLO_CONFIG_H
#define APOLLO_CONFIG_H

class Config {
    public:
        static int APOLLO_COLLECTIVE_TRAINING;
        static int APOLLO_LOCAL_TRAINING;
        static int APOLLO_SINGLE_MODEL;
        static int APOLLO_REGION_MODEL;
        static int APOLLO_TRACE_MEASURES;
        static int APOLLO_NUM_POLICIES;
        static int APOLLO_TRACE_POLICY;
        static int APOLLO_RETRAIN_ENABLE;
        static float APOLLO_RETRAIN_TIME_THRESHOLD;
        static float APOLLO_RETRAIN_REGION_THRESHOLD;
        static int APOLLO_STORE_MODELS;
        static int APOLLO_TRACE_RETRAIN;
        static int APOLLO_TRACE_ALLGATHER;
        static int APOLLO_TRACE_BEST_POLICIES;
        static int APOLLO_FLUSH_PERIOD;
        static int APOLLO_TRACE_CSV;
        static std::string APOLLO_INIT_MODEL;
        static std::string APOLLO_TRACE_CSV_FOLDER_SUFFIX;

    private:
        Config();
};

#endif
