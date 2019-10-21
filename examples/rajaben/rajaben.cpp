
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <set>

#include "util/Color.h"
#include "external/cxxopts/cxxopts.hpp"
//
#include "RAJA/RAJA.hpp"
#include "memoryManager.h"
#include "loopDefinitions.h"
//
#include "apollo/Apollo.h"


template<typename T>
std::ostream &operator <<(std::ostream &os, const std::vector<T> &v) {
    using namespace std;
    copy(v.begin(), v.end(), ostream_iterator<T>(os, "\n"));
    return os;
}

static constexpr
unsigned int hash(const char* str, int h = 0)
{
        return !str[h] ? 5381 : (hash(str, h+1)*33) ^ str[h];
}


class RunSettings {
    public:
        RunSettings() {};
        ~RunSettings() {};

        class Behavior {
            public:
                static constexpr int StaticMax      = 0;
                static constexpr int Sweep          = 1;
                static constexpr int Random         = 2;
        }; //end: class RunSettings::Behavior

        template <typename Arg, typename... Args>
        void log(Arg&& arg, Args&&... args)
        {
            if (verbose == false) return;
            std::cout << "== RAJABEN: ";
            std::cout << std::forward<Arg>(arg);
            using expander = int[];
            (void)expander{0, (void(std::cout << std::forward<Args>(args)), 0)...};
            std::cout << std::endl;
        };

        bool verbose          = false;
        bool sim_sleep        = false;
        int  vector_size_min  = 10000;
        int  vector_size_max  = 10000;
        int  vector_size_step = 1;
        int  iter_max         = -1;
        int  delay_usec       = 0;
        //
        std::vector<std::string>
                    behavior_set;
        std::string behavior_str = "";
        int         behavior       = Behavior::Random;


        std::mt19937 rng;

        // NOTE: See the command line parsing section
        //       below for member descriptions.
        std::string toString(void) const {
            std::ostringstream oss;
            oss << "RunSettings(";
            oss << "verbose:" << verbose << ",";
            oss << "sim_sleep:" << sim_sleep << ",";
            oss << "behavior:" << behavior_str << ",";
            oss << "vector_size_min:"  << vector_size_min << ",";
            oss << "vector_size_max:"  << vector_size_max << ",";
            oss << "vector_size_step:" << vector_size_step << ",";
            oss << "iter_max:" << iter_max << ",";
            oss << "delay_usec:" << delay_usec << ")";
            return oss.str();
        };
        friend std::ostream& operator << (std::ostream &strm, const RunSettings &rs) {
            strm << rs.toString();
            return strm;
        };
};

RunSettings parse(int argc, char **argv) {

    RunSettings run;

    try {
        cxxopts::Options options(argv[0], "");
        options.positional_help("[optional args]");
        options.show_positional_help();

        options
            .add_options()
            ("h,help",
                "Display this message")
            ("v,verbose",
                "Display verbose status messages",
                cxxopts::value<bool>()
                    ->default_value("false"))
            ;

        options
            .add_options("Experiment")
            ("b,behavior",
                "How to explore parameter space [static-max|sweep|random]",
                cxxopts::value<std::string>()
                    ->default_value("random"))
            ("i,iter-max",
                "Limit to the # of times Apollo region is iterated",
                cxxopts::value<int>()->default_value("-1"))
            ("n,vector-size-min",
                "Min. size of the (int) vector",
                cxxopts::value<int>()->default_value("10000"))
            ("x,vector-size-max",
                "Max. size of the (int) vector",
                cxxopts::value<int>()->default_value("10000"))
            ("s,vector-size-step",
                "Growth amount when sweeping multiple (int) vector lengths",
                cxxopts::value<int>()->default_value("1"))
            ("d,delay-usec",
                "Unmeasured delay between RAJA loop iterations",
                cxxopts::value<int>()->default_value("0"))
            ;

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Experiment"}) << std::endl;
            exit(EXIT_SUCCESS);
        }

        result.count("verbose");

        run.behavior_set.push_back("static-max");
        run.behavior_set.push_back("sweep");
        run.behavior_set.push_back("random");


        run.verbose          = result["verbose"].as<bool>();
        run.behavior_str     = result["behavior"].as<std::string>();
        run.iter_max         = result["iter-max"].as<int>();
        run.vector_size_min  = result["vector-size-min"].as<int>();
        run.vector_size_max  = result["vector-size-max"].as<int>();
        run.vector_size_step = result["vector-size-step"].as<int>();
        run.delay_usec       = result["delay-usec"].as<int>();

        // Validate
        if ((run.vector_size_min > run.vector_size_max)
         || (run.vector_size_min < 1)
         || (run.vector_size_step < 1))
        {
            std::cout << "== RAJABEN: [ERROR] Vector sizes supplied to not make sense." << std::endl;
            exit(EXIT_FAILURE);
        }
        if (std::find(std::begin(run.behavior_set), std::end(run.behavior_set),
                    run.behavior_str) != std::end(run.behavior_set)) {
            // Valid behavior selected
            RunSettings::Behavior b;
            switch (hash(run.behavior_str.c_str())) {
                case hash("static-max"): run.behavior = RunSettings::Behavior::StaticMax; break;
                case hash("sweep"):      run.behavior = RunSettings::Behavior::Sweep;     break;
                case hash("random"):     run.behavior = RunSettings::Behavior::Random;    break;
                default: run.behavior = RunSettings::Behavior::Random; break;
            }
        } else {
            // Invalid behavior specified
            std::cout << "== RAJABEN: [ERROR] Behavior not supported: " << run.behavior_str << std::endl;
            std::cout << "== RAJABEN: Please select from the following options:\n\t" << run.behavior_set << std::endl;
            exit(EXIT_FAILURE);
        }

    } catch (const cxxopts::OptionException &e) {
        std::cout << "== RAJABEN: [ERROR] Could not parse command line options.\n\n\t" << e.what() << "\n" << std::endl;
        std::cout << "== RAJABEN: Run again with -h or --help for more information." << std::endl;
        exit(EXIT_FAILURE);
    }

    run.rng.seed(std::random_device()());

    if (run.verbose) {
        std::cout << "== RAJABEN: " << run << std::endl;
    }

    return run;
}


void experimentLoop(Apollo *apollo, RunSettings run) {
    std::uniform_int_distribution<uint32_t>
        random_sizes(run.vector_size_min, run.vector_size_max);
    RunSettings::Behavior b;

    Apollo::Region *reg = new Apollo::Region(apollo, "rajaben", policy_count);

    uint64_t prior_model_guid = 0;
    int      sweep_policy     = 0;
    bool     sweep_complete   = false;
    int      vector_size      = 1;
    int      policy_index     = 0;

    std::string sweep_string = "";
    std::string model_name   = "";

    switch (run.behavior) {
        case RunSettings::Behavior::StaticMax: vector_size = run.vector_size_max;   break;
        case RunSettings::Behavior::Sweep:     vector_size = run.vector_size_min;   break;
        case RunSettings::Behavior::Random:    vector_size = random_sizes(run.rng); break;
        default: vector_size = run.vector_size_max;
    }

    int iter = 1;
    while (true) {


        // Allocate and initialize vector data (untimed)
        //
        int *a = memoryManager::allocate<int>(vector_size);
        int *b = memoryManager::allocate<int>(vector_size);
        int *c = memoryManager::allocate<int>(vector_size);
        for (int p = 0; p < vector_size; p++) {
            a[p] = -p;
            b[p] = p;
        }

        // ##########
        // #
        // #
        // We evaluate our model and express features within the timed
        // portion of the region, as there is no simple way to have those
        // features affiliated with the actual performance of the loop.
        // During training the model evaluates extremely fast, so this is
        // not a significant issue.
        reg->begin();
        // Express our current application-defined features to Apollo
        // for use when evaluating the performance model
        apollo->setFeature("vector_size", (double) vector_size);

        // Evaluate our model and get a policy recommendation
        policy_index = reg->getPolicyIndex();

        if ((reg->getModel()->isTraining())
         && (run.behavior == RunSettings::Behavior::Sweep)
         && (not sweep_complete)) {
            // If we're training during a sweep, override any
            // suggested policy with the one we need to be taking
            // measurements of
            policy_index = sweep_policy;
            sweep_string = BOLD(FBLU("SWEEP "));
        } else {
            sweep_string = "";
        }
        apollo->setFeature("policy_index", (double) policy_index);

        if (reg->getModel()->isTraining()) {
            model_name = sBOLD + sRED + reg->getModel()->getName() + sRST;
        } else {
            model_name = sBOLD + sGRN + reg->getModel()->getName() + sRST;
        }

        run.log(iter, ": ", sweep_string, model_name,
                "[", prior_model_guid, "] --> if (vector_size == ", vector_size,
                ") { policy_index == ", policy_index, "; }");

        //
        // Run the recommended RAJA kernel:
        rajabenPolicySwitcher(
            policy_index,
            [=] (auto exec_policy) {
            RAJA::forall(exec_policy, (RAJA::RangeSegment(0, vector_size) ), [=] (int j)
            {
                //Do the work of the kernel here.
                c[j] = a[j] + b[j];

            });
        });
        reg->end();
        // #
        // #
        // ##########

        memoryManager::deallocate(a);
        memoryManager::deallocate(b);
        memoryManager::deallocate(c);


        // Get the parameters for the next loop iteration:
        switch (run.behavior) {
            case RunSettings::Behavior::StaticMax: vector_size = run.vector_size_max; break;
            case RunSettings::Behavior::Sweep:
                // If we have a new model, reset the sweep:
                if (prior_model_guid != reg->getModel()->getGuid()) {
                    run.log(BOLD(FMAG("======== NEW MODEL : SWEEP RESET ========")));
                    vector_size = run.vector_size_min;
                    sweep_policy = 0;
                    sweep_complete = false;
                    prior_model_guid = reg->getModel()->getGuid();
                } else {
                    vector_size += run.vector_size_step;
                    if (vector_size > run.vector_size_max) {
                        vector_size = run.vector_size_min;
                        sweep_policy++;
                        if (sweep_policy >= policy_count) {
                            sweep_policy   = 0;
                            sweep_complete = true;
                        }
                    }
                }
                break;
            case RunSettings::Behavior::Random: vector_size = random_sizes(run.rng); break;
            default:       vector_size = run.vector_size_max;
        } // end: select(behavior)

        // Count this iteration, break out of the experiment loop if needed:
        iter++;
        if (run.iter_max > 0) {
            // We're not set to run forever, see if we're ready to terminate:
            if (run.behavior == RunSettings::Behavior::Sweep) {
                // We need to make sure we have at least one
                // complete sweep of the parameters
                if ((sweep_complete)
                 && (iter > run.iter_max)) {
                    break; // leave the loop
                }
            } else {
                // We're not sweeping, so if we've hit enough iterations, we're done
                if (iter > run.iter_max) {
                    break; //leave the loop
                }
            }
        }
        // If an "untimed" per-iteration delay was requested, sleep now
        std::this_thread::sleep_for(std::chrono::microseconds(run.delay_usec));
    } // end: iteration loop

    delete reg; // tear down the region
    return;
}

int main(int argc, char **argv)
{
    auto run = parse(argc, argv);
    Apollo *apollo = Apollo::instance();

    experimentLoop(apollo, run);

    return EXIT_SUCCESS;
}


