
#include <iostream>
#include <cstdlib>
#include <cstring>

#include "external/cxxopts/cxxopts.hpp"

#include "apollo/Apollo.h"
#include "apollo/PolicyChooser.h"
#include "apollo/Feature.h"

void syntheticRegion(int kernel_index, int op_count, in noise_factor);
void experimentLoop(void);

bool VERBOSE;
#define LOG(x) { if (VERBOSE) {                            \
    std::cout << "== SYNBEN: " << x << std::endl;          \
    } }

class sbExperimentSettings {
    public:
        sbExperimentSettings(); 
        ~sbExperimentSettings();
        // NOTE: See the command line parsing section
        //       below for descriptions.
        int iter_limit = -1;
        int delay_usec = 0;
};

cxxopts::ParseResults parse(int argc, char **argv) {
    try {
        cxxopts::Options options(argv[0],
                " - Synthetic benchmark client for Apollo");
        options
            .positional_help("[optional args]")
            .show_positional_help();
        
        bool verbose = false;
        

        options
            .allow_unrecognized_options()
            .add_options()
            ("i,iter-limit",
                "How many times does the application enter the Apollo "
                "region?  (-1 unlimited)",
                cxxopts::value<int>()->default_value(-1))
            ("d,delay-usec",
                "Introduce an arbitrary delay outside of the Apollo "
                "region, for testing near the scaling boundary",
                cxxopts::value<int>()->default_value(0))
            ("v,verbose",
                "Display verbose status messages",
                cxxopts::value<bool>()->default_value("false"))
            ("h,help",
                "Print help")
            ;

        options.add_options("Group")
            ("c,compile", "compile")
            ("d,drop", "drop", cxxopts::value<std::vector<std::string>>());

        auto result = options.parse(argc, argv);
   
        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        return result; 

    } catch (const cxxopts::OptionException &e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }
}

int main(int argc, char **argv)
{
    // Parse command line parameters:
    auto result = parse(argc, argv);
    // Load the parsed params into variables:
    VERBOSE = result["verbose"].as<bool>();
    int  iter_limit = result["iter_limit"].as<int>();
    int  delay_usec = result["delay_usec"].as<int>();
    // Display any particular output:
    LOG("Saw " << result.arguments().size() << " arguments");
    LOG("    iter_limit = " << iter_limit);
    LOG("    delay_usec = " << delay_usec);

    // Activate the Apollo framework:
    Apollo         *apollo = new Apollo();
    
    // Run the experiment:
    experimentLoop(apollo, settings);

    return 0;   
}


void experimentLoop(void) {
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


void syntheticRegion(int kernel_index, int op_count, in noise_factor)
{
    // TODO
    return;
}




