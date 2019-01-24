
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

#include "external/cxxopts/cxxopts.hpp"

#include "apollo/Apollo.h"
#include "apollo/PolicyChooser.h"
#include "apollo/Feature.h"

void configureKernels(void);
int  syntheticRegion(auto& run, auto& kernel, int t_noise);
void experimentLoop(Apollo *apollo, auto& run);


class Kernel {
    public:
        Kernel() {};
        Kernel(int set_t_fixed, int set_t_op) {
            t_fixed = set_t_fixed;
            t_op    = set_t_op;
        };
        ~Kernel() {};

        int t_fixed       = 0;
        int t_op          = 1;
        int noise_factor  = 0;

        std::string toString(void) const {
            std::ostringstream oss;
            oss << "Kernel(";
            oss << "t_fixed:" << t_fixed << ",";
            oss << "t_op:" << t_op << ")";
            return oss.str();
        };
        friend std::ostream& operator << (std::ostream &strm, const Kernel &k) {
            strm << k.toString();
            return strm;
        };
};


class RunSettings {
    public:
        RunSettings() {}; 
        ~RunSettings() {};
        
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
        bool random_ops     = false;
        int  op_count_max   = 0;
        int  iter_max       = -1;
        int  noise_usec_max = 0;
        int  delay_usec     = 0;

        std::vector<Kernel> kernels;

        std::mt19937 rng;

        // NOTE: See the command line parsing section
        //       below for member descriptions.
        std::string toString(void) const {
            std::ostringstream oss;
            oss << "RunSettings(";
            oss << "verbose:" << verbose << ",";
            oss << "sim_sleep:" << sim_sleep << ",";
            oss << "random_ops:" << random_ops << ",";
            oss << "op_count_max:" << op_count_max << ",";
            oss << "iter_max:" << iter_max << ",";
            oss << "noise_usec_max:" << noise_usec_max << ",";
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
            ("r,random-ops",
                "Randomize number of operations per iteration",
                cxxopts::value<bool>()
                    ->default_value("false"))
            ("o,op-count-max",
                "Maximum operations to be simulated in the Apollo region",
                cxxopts::value<int>()->default_value("2000"))
            ("i,iter-max",
                "Limit to the # of times Apollo region is iterated",
                cxxopts::value<int>()->default_value("-1"))
            ("n,noise-usec-max",
                "Random usec between 0 and noise-max per iteration",
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

        run.verbose        = result["verbose"].as<bool>();
        run.sim_sleep      = result["sim-sleep"].as<bool>();
        run.random_ops     = result["random-ops"].as<bool>();
        run.op_count_max   = result["op-count-max"].as<int>();
        run.iter_max       = result["iter-max"].as<int>();
        run.noise_usec_max = result["noise-usec-max"].as<int>();
        run.delay_usec     = result["delay-usec"].as<int>();

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

void configureKernels(auto& run) {
    run.kernels.push_back(Kernel(1000, 5));
    run.kernels.push_back(Kernel(2000, 1));
    if (run.verbose) { for (const auto& k : run.kernels) { run.log(k); }}
}


int main(int argc, char **argv)
{
    auto run = parse(argc, argv);    
    Apollo *apollo = new Apollo();
    configureKernels(run);
    experimentLoop(apollo, run);

    return EXIT_SUCCESS;
}


void experimentLoop(Apollo *apollo, auto& run) {
    std::uniform_int_distribution<uint32_t>
        noise(0, std::max((int)run.noise_usec_max, (int)1));
    std::uniform_int_distribution<uint32_t>
        ops(1, std::max((int)run.op_count_max, (int)2));

    Apollo::Region *reg =
        new Apollo::Region(apollo, "synben", run.kernels.size());
    reg->begin();

    int pol_idx = 0;
    int t_total = 0;
    int t_noise = 0;
    int op_count = 1;

    int i = 1;
    while (true) {
        pol_idx = getApolloPolicyChoice(reg);
        auto kernel = run.kernels.at(pol_idx);
        t_noise = std::min((int)noise(run.rng), (int)run.noise_usec_max);
        if (run.random_ops) {
            op_count = ops(run.rng);
        } else {
            op_count = run.op_count_max;
        }
        // -----
        reg->iterationStart(i);
            t_total = syntheticRegion(run, kernel, op_count, t_noise);
            reg->caliSetInt("t_total", t_total);
            reg->caliSetInt("t_noise", t_noise);
            reg->caliSetInt("op_count", op_count);
        reg->iterationStop();
        // -----
        run.log("Iteration ", i, " of ", run.iter_max,
                ": kernel ", pol_idx, " took ", t_total,
                " usec to process ", op_count, " ops");
        //
        i++; if ((run.iter_max > 0) && (i > run.iter_max)) { break; }
        std::this_thread::sleep_for(std::chrono::microseconds(run.delay_usec));
    }
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




