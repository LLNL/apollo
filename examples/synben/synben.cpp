
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "external/cxxopts/cxxopts.hpp"

#include "apollo/Apollo.h"
#include "apollo/PolicyChooser.h"
#include "apollo/Feature.h"

void syntheticRegion(int kernel_index, int op_count, int noise_factor);
void experimentLoop(Apollo *apollo, auto run);

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

        // NOTE: See the command line parsing section
        //       below for descriptions.
        bool verbose = false;
        int iter_limit = -1;
        int delay_usec = 0;
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
            ("i,iter-limit",
                "Limit to the # of times Apollo region is iterated",
                cxxopts::value<int>()->default_value("-1"))
            ("d,delay-usec",
                "Unmeasured delay between Apollo region iterations",
                cxxopts::value<int>()->default_value("0"))
            ;

        auto result = options.parse(argc, argv);
   
        if (result.count("help")) {
            std::cout << options.help({"", "Experiment"}) << std::endl;
            exit(0);
        }

        result.count("verbose");

        run.verbose = result["verbose"].as<bool>();
        run.iter_limit = result["iter-limit"].as<int>();
        run.delay_usec = result["delay-usec"].as<int>();


    } catch (const cxxopts::OptionException &e) {
        std::cout << "== SYNBEN: [ERROR] Could not parse command line options.\n\n\t" << e.what() << "\n" << std::endl;
        std::cout << "== SYNBEN: Run again with -h or --help for more information." << std::endl;
        exit(1);
    }

    return run;
}

int main(int argc, char **argv)
{
    // Parse command line parameters:
    auto run = parse(argc, argv);
    
    // Display any particular output:
    run.log("Run settings:");
    run.log("    iter_limit = ", run.iter_limit);
    run.log("    delay_usec = ", run.delay_usec);

    // Activate the Apollo framework:
    Apollo *apollo = new Apollo();
    
    // Run the experiment:
    experimentLoop(apollo, run);

    return 0;   
}


void experimentLoop(Apollo *apollo, auto run) {
    //Apollo::Region *kernel = new Apollo::Region(apollo, "sb", 2);

    // Example: kernel->caliSetInt("vector_size", N);

    // TODO: Do we still need both of these?
    //kernel->begin();
    //kernel->iterationStart(increasing);
    
    // TODO: Call on the model to provide an index.
    //getApolloPolicyChoice(kernel),
    
    
    // TODO: (see above) Do we still need both of these?
    //kernel->iterationStop();
    //kernel->end();
     return;
}


void syntheticRegion(int kernel_index, int op_count, int noise_factor)
{
    // TODO
    return;
}




