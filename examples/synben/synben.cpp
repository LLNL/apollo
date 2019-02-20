
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

#include "external/cxxopts/cxxopts.hpp"

#include "apollo/Apollo.h"
#include "apollo/PolicyChooser.h"
#include "apollo/Feature.h"

void configureKernelVariants(void);
int  syntheticKernelVariantRecommendation(int op_count);

int  syntheticRegion(auto& run, auto& kernel_variant, int t_op_weight);
void experimentLoop(Apollo *apollo, auto& run);

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

class KernelVariant {
    public:
        KernelVariant() {};
        KernelVariant(int set_t_fixed, int set_t_op, std::string set_name) {
            t_fixed = set_t_fixed;
            t_op    = set_t_op;
            name    = set_name;
        };
        ~KernelVariant() {};

        int         t_fixed       = 0;
        int         t_op          = 1;
        std::string name          = "";
        std::string toString(void) const {
            std::ostringstream oss;
            oss << "Kernel(";
            oss << "t_fixed:" << t_fixed << ",";
            oss << "t_op:" << t_op << ",";
            oss << "name:\"" << name << "\")";
            return oss.str();
        };
        friend std::ostream& operator << (std::ostream &strm, const KernelVariant &k) {
            strm << k.toString();
            return strm;
        };
};


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
            std::cout << "== SYNBEN: ";
            std::cout << std::forward<Arg>(arg);
            using expander = int[];
            (void)expander{0, (void(std::cout << std::forward<Args>(args)), 0)...};
            std::cout << std::endl;
        };

 
        bool verbose        = false;
        bool sim_sleep      = false;
        int  op_count_max   = 0;
        int  op_weight_max  = 0;
        int  iter_max       = -1;
        int  delay_usec     = 0;
        //
        std::vector<std::string>
                    behavior_set;
        std::string behavior_str = "";
        int         behavior       = Behavior::Random;


        std::vector<KernelVariant> kernel_variants;

        std::mt19937 rng;

        // NOTE: See the command line parsing section
        //       below for member descriptions.
        std::string toString(void) const {
            std::ostringstream oss;
            oss << "RunSettings(";
            oss << "verbose:" << verbose << ",";
            oss << "sim_sleep:" << sim_sleep << ",";
            oss << "behavior:" << behavior_str << ",";
            oss << "op_count_max:" << op_count_max << ",";
            oss << "op_weight_max:" << op_weight_max << ",";
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
            ("s,sim-sleep",
                "Directly set times rather than actually sleeping",
                cxxopts::value<bool>()
                    ->default_value("false"))
            ("b,behavior",
                "How to explore parameter space [static-max|sweep|random]",
                cxxopts::value<std::string>()
                    ->default_value("random"))
            ("i,iter-max",
                "Limit to the # of times Apollo region is iterated",
                cxxopts::value<int>()->default_value("-1"))
            ("c,op-count-max",
                "Maximum operations to be simulated in the Apollo region",
                cxxopts::value<int>()->default_value("2000"))
            ("w,op-weight-max",
                "Operation cost units (usec) between 0 and op-weight-max per"
                " operation, randomized between iterations",
                cxxopts::value<int>()->default_value("0"))
            ("d,delay-usec",
                "Unmeasured delay between Apollo region iterations",
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


        run.verbose        = result["verbose"].as<bool>();
        run.sim_sleep      = result["sim-sleep"].as<bool>();
        run.behavior_str   = result["behavior"].as<std::string>();
        run.op_count_max   = result["op-count-max"].as<int>();
        run.op_weight_max  = result["op-weight-max"].as<int>();
        run.iter_max       = result["iter-max"].as<int>();
        run.delay_usec     = result["delay-usec"].as<int>();

        // Validate and apply the requested behavior
        if (std::find(std::begin(run.behavior_set), std::end(run.behavior_set),
                    run.behavior_str) != std::end(run.behavior_set)) {
            // Valid behavior selected
            switch (hash(run.behavior_str.c_str())) {
                case hash("static-max"): run.behavior = RunSettings::Behavior::StaticMax; break;
                case hash("sweep"):      run.behavior = RunSettings::Behavior::Sweep;     break;
                case hash("random"):     run.behavior = RunSettings::Behavior::Random;    break;
                default: run.behavior = RunSettings::Behavior::Random; break;
            }
        } else {
            // Invalid behavior specified
            std::cout << "== SYNBEN: [ERROR] Behavior not supported: " << run.behavior_str << std::endl;
            std::cout << "== SYNBEN: Please select from the following options:\n\t" << run.behavior_set << std::endl;
            exit(EXIT_FAILURE);
        }

    } catch (const cxxopts::OptionException &e) {
        std::cout << "== SYNBEN: [ERROR] Could not parse command line options.\n\n\t" << e.what() << "\n" << std::endl;
        std::cout << "== SYNBEN: Run again with -h or --help for more information." << std::endl;
        exit(EXIT_FAILURE);
    }

    run.rng.seed(std::random_device()());

    if (run.verbose) {
        std::cout << "== SYNBEN: " << run << std::endl;
    }

    return run;
}

void configureKernelVariants(auto& run) {
    run.kernel_variants.push_back(KernelVariant(1000,  5, "[cpu     ]")); 
    run.kernel_variants.push_back(KernelVariant(2000,  2, "[pthreads]")); 
    run.kernel_variants.push_back(KernelVariant(4000,  2, "[openmp  ]")); 
    run.kernel_variants.push_back(KernelVariant(4000,  1, "[cuda    ]")); 
    if (run.verbose) { for (const auto& k : run.kernel_variants) { run.log(k); }}
    return;    
}

int syntheticKernelVariantRecommendation(auto& run, int op_count, int op_weight) {
    int this_cost = 0;
    int best_cost_seen = 99999999;
    int best_kernel = -1;
    for (unsigned int i = 0; i < run.kernel_variants.size(); i++) {
        auto kernel = run.kernel_variants[i];
        this_cost = syntheticRegion(run, kernel, op_count, op_weight); 
        if (this_cost < best_cost_seen) {
            best_kernel = (int) i;
            best_cost_seen = this_cost;
        }
    }
    return best_kernel;
}


int main(int argc, char **argv)
{
    auto run = parse(argc, argv);    
    Apollo *apollo = new Apollo();
    configureKernelVariants(run);
    experimentLoop(apollo, run);

    return EXIT_SUCCESS;
}


void experimentLoop(Apollo *apollo, auto& run) {
    std::string runMaxDesc;
    if (run.iter_max > 0) {
        runMaxDesc = " of " + std::to_string(run.iter_max);
    } else {
        runMaxDesc = "";
    }
    std::uniform_int_distribution<uint32_t>
        weights(0, std::max((int)run.op_weight_max, (int)1));
    std::uniform_int_distribution<uint32_t>
        ops(1, std::max((int)run.op_count_max, (int)2));

    Apollo::Region *reg =
        new Apollo::Region(apollo, "synben", run.kernel_variants.size());
    reg->begin();

    int pol_idx = 0;
    int test_pol = 0;

    int t_total = 0;
    int op_count = 1;
    int op_weight = 0;
    int group_id = 0;

    bool optimal_variant_used = false;
    bool sweep_complete       = false; // Only used for sweep behavior

    std::string       sweep_progress;
    std::stringstream sweep_progress_ss;
    sweep_progress = "";
    sweep_progress_ss << "";

    int i = 1;
    while (true) {

        switch (run.behavior) {
            case RunSettings::Behavior::StaticMax:
                op_count = run.op_count_max;
                break;

            case RunSettings::Behavior::Sweep:
                // This will give us at least one of each count X weight in the range specified
                op_count++;
                if (op_count > run.op_count_max) {
                    op_count = 1;
                    op_weight++;
                    if (op_weight > run.op_weight_max) {
                        op_weight = 0;
                        sweep_complete = true;
                        run.log("---- SWEEP COMPLETE ----");
                    }
                }
                break;

            case RunSettings::Behavior::Random:
            default:
                op_count = ops(run.rng);
                op_weight = std::min((int)weights(run.rng), (int)run.op_weight_max);
                break;
        }

        
        group_id = i % 50;
        
        // -----
        reg->iterationStart(i);
            // Express our configuration to Caliper:
            reg->caliSetInt("group_id", group_id);
            reg->caliSetInt("op_count", op_count);
            reg->caliSetInt("t_op_weight", op_weight);
            // Select our "kernel variant":
            pol_idx = getApolloPolicyChoice(reg);

            // Check the kernel variant recommended against the synthetic test:
            test_pol = syntheticKernelVariantRecommendation(run, op_count, op_weight);
            if (test_pol != pol_idx) {
                optimal_variant_used = false;
            } else {
                optimal_variant_used = true;
            }

            reg->caliSetInt("policy_index", pol_idx);
            auto kernel = run.kernel_variants.at(pol_idx);
            reg->caliSetInt("t_op", kernel.t_op);
            // Run it:
            //
            t_total = syntheticRegion(run, kernel, op_count, op_weight);
            //
            
            if (run.behavior == RunSettings::Behavior::Sweep) {
                sweep_progress = "";
                sweep_progress_ss.str(std::string());
                int denom = std::max(1, run.op_count_max) * std::max(1, run.op_weight_max);
                sweep_progress_ss << "SweepProgress(" << std::setprecision(4) << (((float) i / (float) denom) * 100.0) << ") ";
                sweep_progress = sweep_progress_ss.str();
            }

            run.log(sweep_progress, "Iter ", i, runMaxDesc,
                ": k.variant ", pol_idx, " ", kernel.name, " took ", std::left,
                std::setw(5), t_total, " usec on ", std::left, std::setw(5),
                op_count, " ops, optimal == ", optimal_variant_used);

            // Record the computed time, in case we're not actually sleeping
            // and don't want to use the time captured by Caliper:
            reg->caliSetInt("t_total", t_total);
        reg->iterationStop();
        // -----

        // Count this iteration:
        i++;
        if (run.iter_max > 0) {
            // We're not set to run forever, see if we're ready to terminate:
            if (run.behavior == RunSettings::Behavior::Sweep) {
                // We need to make sure we have at least one
                // complete sweep of the parameters
                if ((sweep_complete)
                 && (i > run.iter_max)) {
                    break; // leave the loop
                }
            } else {
                // We're not sweeping, so if we've hit enough iterations, we're done
                if (i > run.iter_max) {
                    break; //leave the loop
                }
            }
        }
        // Increment an arbitrary group_id field to for easy optional
        // subdivision of data later on during analysis:
        if (not (i % 100)) { group_id++; };
        // If an "untimed" per-iteration delay was requested, sleep now
        std::this_thread::sleep_for(std::chrono::microseconds(run.delay_usec));
    } // end: iteration loop

    reg->end();
    return;
}


int syntheticRegion(auto& run, auto& kernel, int op_count, int t_noise)
{
    int t_total = kernel.t_fixed + (op_count * (kernel.t_op + t_noise));
    if (not run.sim_sleep) {
        std::this_thread::sleep_for(std::chrono::microseconds(t_total));
    }
    return t_total;
}




