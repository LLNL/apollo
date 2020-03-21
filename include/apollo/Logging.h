#ifndef APOLLO_LOGGING_H
#define APOLLO_LOGGING_H

#include <cstring>
#include <string>
#include <utility>
#include <iostream>
#include <type_traits>

#include "apollo/Apollo.h"

#ifndef APOLLO_VERBOSE
#define APOLLO_VERBOSE 0
#endif

#ifndef APOLLO_LOG_FIRST_RANK_ONLY
#define APOLLO_LOG_FIRST_RANK_ONLY 1
#endif

/*
 *  TODO: [LOGGING] This class / mechanism is pending.
 *        Ignore this for now.  -Chad
 *
class Apollo::Logging
{
    public:
        ~Logging();
        Logging(const Logging&) = delete;
        Logging& operator=(const Logging&) = delete;

        enum class Style : int {
            TEXT,
            CSV,
            SOS
        };

        class Channel {
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;

            std::string              name;
            bool                     is_active;
            bool                     is_first_rank_only;
            bool                     is_echoed_to_stdout;
            bool                     is_synchronous;
            bool                     is_locking;
            int                      filter_level;
            Apollo::Logging::Style   style;
            std::vector<std::string> output_async_cache;
            std::fstream             output_stream;
            std::mutex               channel_lock;
        };

        bool  is_persistent_override;
        int   persistent_override_to_filter_level;
        bool  persistent_override_to_active;

        std::map<std::string, Apollo::Logging::Channel> channels;
        std::mutex                                      logging_config_lock;

        void create(std::string name;)
        void create(std::string name, int filter_level);
        void create(std::string name, int filter_level, Apollo::Logging::Style style);
        void create(std::string name, int filter_level, Apollo::Logging::Style style,
                bool first_rank_only, bool echoed_to_stdout, bool synchronous, bool locking);

        // Applies to all existing and any new channels, and cannot be changed
        // by API calls to setLevel() or setActive() for a channel. Used for globally
        // enabling, deepening, or disabling logging, while allowing specific code
        // areas to maintain appropriate default levels of verbosity and to
        // branch into overhead-inducing blocks of code to gather their log data
        // based on the log level of their specific channel.
        void setPersistentOverride(int global_level, bool globally_active);

        // Apply to all existing channels:
        void flush(void);
        void setLevel(int level);
        void setActive(bool is_active);
        void close(void);

        // Apply to specific channels:
        void flush(std::string name);
        void setLevel(std::string name, int level);
        void setActive(std::string name, bool is_active);
        void close(std::string name);


};

//TODO: [LOGGING] Change this template to strip off the first two elements
//      as channel name and level, then feed the rest into another template
//      that returns a string, which this handles differently based on the
//      channel's logging style.

*/

template <typename Arg, typename... Args>
void log(Arg&& arg, Args&&... args)
{
    if (APOLLO_VERBOSE < 1) return;

    static int rank = -1;
    if (rank < 0) {
        const char *rank_str = getenv("SLURM_PROCID");
        if ((rank_str != NULL) && (strlen(rank_str) > 0))
        {
            rank = atoi(rank_str);
        }
    }

    if (APOLLO_LOG_FIRST_RANK_ONLY > 0)
    {
        if (rank != 0) return;
    }

    std::cout << "== APOLLO(" << rank << "): ";
    std::cout << std::forward<Arg>(arg);
    using expander = int[];
    (void)expander{0, (void(std::cout << std::forward<Args>(args)), 0)...};
    std::cout << std::endl;
};


#endif
