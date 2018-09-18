
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <mutex>

#include "apollo/Apollo.h"
#include "apollo/models/DecisionTree.h"

#define modelName "decisiontree"
#define modelFile __FILE__

int
Apollo::Model::DecisionTree::getIndex(void)
{
    std::lock_guard<std::mutex> lock(modelMutex);
    static int choice = -1;
    //
    iterCount++;
    
    if (configured == false) {
        if (iterCount < 10) {
            fprintf(stderr, "ERROR: DecisionTree::getIndex() called prior to"
                " model configuration. Defaulting to index 0.\n");
            fflush(stderr);
        } else if (iterCount == 10) {
            fprintf(stderr, "ERROR: DecisionTree::getIndex() has still not been"
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
Apollo::Model::DecisionTree::configure(
        Apollo      *apollo_ptr,
        int          numPolicies,
        const char  *model_definition)
{
    //NOTE: Make sure to grab the lock from the calling code:
    //          std::lock_guard<std::mutex> lock(model->modelMutex);

    apollo      = apollo_ptr;
    policyCount = numPolicies;

    if (configured == true) {
        // TODO: This is a RE-configuration. Remove previous configuration.
        // ...
        // ...
        configured = false;
        tree_features.clear();
        tree_nodes.clear();
        tree_head = nullptr;
        model_def = "";
    }

    // Construct a decisiontree for this model_definition.
    if (model_definition == NULL) {
        fprintf(stderr, "WARNING: Cannot successfully configure"
                " with a NULL or empty model definition.\n");
        model_def = "";
        configured = false;
        return;
    }

    model_def = model_definition;

    std::istringstream model(model_def);
    typedef std::stringstream  unpack_str;
    std::string        line;

    int num_features = 0;

    // Find out how many features are named:
    std::getline(model, line);
    unpack_str(line) >> num_features;

    // Load in the feature names:   (values are fetched by getIndex())
    for (int i = 0; i < num_features; i++) {
        std::getline(model, line);
        tree_features.push_back(std::string(line));
    }

    // Load in the rules:
    int         line_id   = -1;
    std::string line_feat = NULL;
    std::string line_op   = NULL;
    double      line_val  = 0;

    Node *node = nullptr;

    while (std::getline(model, line)) {
        unpack_str(line) >> line_id >> line_feat >> line_op >> line_val;

        auto tree_find = tree_nodes.find(line_id);
        //
        // Either we are adding to an existing node, or a new one, either way
        // make 'node' point to the correct object instance.
        if (tree_find == tree_nodes.end()) { 
                node = new Node(apollo, line_id, line_feat.c_str());
                tree_nodes.emplace(line_id, node);
        } else {
                node = tree_find->second;
        }

        // Set the components of this node, give the content of this line.
        // NOTE: apollo, node_id, and node_feat have already been set.
        if (line_op == "<=") {
            break;
        } else if (line_op == ">") {
            break;
        } else if (line_op == "=") {
            break;
        } else {
            fprintf(stderr, "ERROR: Unable to process decision"
                    " tree model definition (bad operator"
                    " encountered).\n");
            configured = false;
            return; 
        } // end: if(node_op...)

        //...
    }

    // TODO: Traverse the nodes and assemble them together into a
    //       binary tree. This should be doable given the linear indexing
    //       of the nodes.

    configured = true;
    return;
}
//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::DecisionTree::DecisionTree()
{
    iterCount = 0;
}

Apollo::Model::DecisionTree::~DecisionTree()
{
    return;
}

extern "C" Apollo::Model::DecisionTree*
APOLLO_model_create_decisiontree(void)
{
    return new Apollo::DecisionTree;
}


extern "C" void
APOLLO_model_destroy_decisiontree(
        Apollo::Model::DecisionTree *tree_ref)
{
    delete tree_ref;
    return;
}


