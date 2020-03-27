#ifndef APOLLO_LOGGING_H
#define APOLLO_LOGGING_H

#include <cstring>
#include <string>
#include <utility>
#include <iostream>
#include <type_traits>

#ifndef APOLLO_VERBOSE
#define APOLLO_VERBOSE 0
#endif

#ifndef APOLLO_LOG_FIRST_RANK_ONLY
#define APOLLO_LOG_FIRST_RANK_ONLY 1
#endif

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
