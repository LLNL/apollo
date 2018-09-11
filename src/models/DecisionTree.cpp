
#include <map>
#include <string>
#include <sstream>
#include <iostream>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

#include "apollo/common/DecisionNode.h"


// 
// ----------
//
// MODEL: This is where any INDEPENDENT variables get checked
//        and a policy decision is made.
//

#define modelName "sequential"
#define modelFile __FILE__

Apollo::DecisionNode<double, int>                             *tree_head;
std::map<int, Apollo::DecisionNode<double, int> *>             tree_nodes;
std::map<int, Apollo::DecisionNode<double, int> *>::iterator   tree_find;
std::list<std::string>                                         tree_features;

int
Apollo::Model::getIndex(void)
{
    static int choice = -1;
    //
    iterCount++;
    
    if (configured == false) {
        if (iterCount < 10) {
            fprintf(stderr, "ERROR: Model::getIndex() called prior to"
                " model configuration. Defaulting to index 0.\n");
            fflush(stderr);
        } else if (iterCount == 10) {
            fprintf(stderr, "ERROR: Model::getIndex() has still not been"
                    " configured. Continuing default behavior without further"
                    " error messages.\n");
        }
        choice = 0;
        return choice;
    }

    // NOTE: Here we need to check the values of the independent
    //       variables to plug into the decision tree solver.
    //
    //       Consensus:
    //          "Pull current values from Caliper via simple Apollo API call."
    //       Mechanism:
    //          //This happens in the base class:
    //          dectree = deserializeDecTree(stringFromPythonController)
    //          inputBlackboard = apollo->getFeaturesByName(dectree->names[]);
    //          (then you can use inputBlackboard to look up values named in
    //          the decision tree...)


    return choice;
}

// Process a decision tree definition (string encoding) into
// a functioning decision tree model for use with getIndex.

void
Apollo::Model::configure(
        Apollo      *apollo_ptr,
        int          numPolicies,
        const char  *model_definition)
{
    apollo      = apollo_ptr;
    policyCount = numPolicies;

    if (configured == true) {
        // TODO: This is a RE-configuration. Remove previous configuration.
        // ...
        // ...
        tree_features.clear();
        tree_nodes.clear();
        tree_head = nullptr;
        delete model_def;
    }

    // Construct a decisiontree for this model_definition.
    if (model_definition == NULL) {
        fprintf(stderr, "WARNING: Cannot successfully configure"
                " with a NULL or empty model definition.\n");
        model_def = new std::string("");
        configured = false;
        return false;
    }

    model_def = new std::string(model_definition);

    std::istringstream model(model_def);
    std::string line;

    int num_features = 0;
    
    // Find out how many features are named:
    std::getline(model, line);
    sscanf(line.c_str(), "%d", &num_features);

    // Load in the feature names:   (values are fetched by getIndex())
    for (int i = 0; i < num_features; i++) {
        std::getline(model, line);
        tree_features.push_back(new std::string(line));
    }

    // Load in the rules:
    int         line_id   = -1;
    std::string line_feat = NULL;
    std::string line_op   = NULL;
    double      line_val  = 0;

    Apollo::DecisionNode<double, int> *new_node = nullptr;

    while (std::getline(model, line)) {
        std::stringstream unpack_line(line);
        unpack_line >> node_id >> node_feat
             >> node_op >> node_val;

        tree_find = tree_nodes.find(node_id);
        //
        if (tree_find == tree_nodes.end()) { 
                node = new Apollo::DecisionNode<double, int>(
                            apollo, node_id, node_feat);
                tree_nodes.emplace(node_id, node);
        } else {
                node = tree_find.first();
        }

        // Set the components of this node, give the content of this line.

        // TODO: Traverse the nodes and assemble them together into a
        //       binary tree. This should be doable given the linear indexing
        //       of the nodes.

    }
     


    configured = true;
    return configured;
}
//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::Model()
{
    iterCount = 0;
}

Apollo::Model::~Model()
{
    return;
}

extern "C" Apollo::Model* create_instance(void)
{
    return new Apollo::Model;
}


extern "C" void destroy_instance(Apollo::Model *model_ref)
{
    delete model_ref;
    return;
}


