
#include <map>
#include <string>
#include <sstream>
#include <iostream>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "caliper/cali.h"
#include "caliper/common/cali_types.h"

#include "apollo/Apollo.h"
#include "apollo/models/DecisionTree.h"

#define modelName "decisiontree"
#define modelFile __FILE__


int
Apollo::Model::DecisionTree::recursiveTreeWalk(Node *node) {
    // Compare the node->value to the defined comparison values
    // and either dive down a branch or return the choice up.
    if (node->feature->value <= node->leq_val) {
        if (node->leftChild == nullptr) {
            return node->recommendation;
        } else {
            return recursiveTreeWalk(node->leftChild);
        }
    }
    if (node->feature->value > node->grt_val) {
        if (node->rightChild == nullptr) {
            return node->recommendation;
        } else {
            return recursiveTreeWalk(node->rightChild);
        }
    }
    return node->recommendation;
} //end: recursiveTreeWalk(...)   [function]


int
Apollo::Model::DecisionTree::getIndex(void)
{
    // Keep choice around for easier debugging, if needed:
    static int choice = -1;

    iter_count++;
    if (configured == false) {
        if (iterCount < 10) {
            fprintf(stderr, "[ERROR] DecisionTree::getIndex() called prior to"
                " model configuration. Defaulting to index 0.\n");
            fflush(stderr);
        } else if (iterCount == 10) {
            fprintf(stderr, "[ERROR] DecisionTree::getIndex() has still not been"
                    " configured. Continuing default behavior without further"
                    " error messages.\n");
        }        
        // Since we're not configured yet, return a default:
        choice = 0;
        return choice;
    }

    // Refresh the values for each feature mentioned in the decision tree.
    // This gives us coherent tree behavior for this iteration, even if the model
    // gets replaced or other values are coming into Caliper during this process,
    // values being evaluated wont change halfway through walking the tree:
    bool converted_ok = true;
    for (Feature *feat : tree_features) {
        feat->value_variant = cali_get(feat->cali_id);
        feat->value         = cali_variant_to_double(feat->value_variant, &converted_ok);
        if (not converted_ok) {
            fprintf(stderr, "== APOLLO: [ERROR] Unable to convert feature to a double!\n");
        }
    }

    // Find the recommendation of the decision tree:
    choice = recursiveTreeWalk(tree_head);

    return choice;
}


void
Apollo::Model::DecisionTree::configure(
        Apollo      *apollo_ptr,
        int          numPolicies,
        std::string  model_definition)
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
        tree_head = nullptr;
        for (Node *node : tree_nodes) {
            free(node);
        }
        tree_nodes.clear();
        for (Feature *feat : tree_features) {
            free(feat);
        }
        tree_features.clear();

        model_def = "";
    }

    // Construct a decisiontree for this model_definition.
    if (model_definition == "") {
        fprintf(stderr, "[WARNING] Cannot successfully configure"
                " with a NULL or empty model definition.\n");
        model_def = "";
        configured = false;
        return;
    }

    model_def = model_definition;

    //TODO: Unpack the JSON of the tree.
    
    //TODO: Something like below, initalize the nodes and attach whatever
    //      feature each one refers to.  If that feature doesn't exist,
    //      initialize it and add it to the list of features.
    auto feature_id_find = tree_features.find(feat_name);
    if (feature_id_find == tree_features.end()) {
        // We're not tracking this feature in our accelleration
        // structure yet..
        feat_id = cali_find_attribute(feat_name.c_str());
        if (feat_id == CALI_INV_ID) {
            // Error out: Somehow the DecisionTree told us to split on a
            // value that is not in our Caliper data, in a model built
            // using only our Caliper data. This can be a very specific
            // error message, likely a hard fail for debugging.
            apollo_log(); 
            exit(EXIT_FAILURE);
        }
        // TODO: Put in the in the map
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
    return new Apollo::Model::DecisionTree();
}


extern "C" void
APOLLO_model_destroy_decisiontree(
        Apollo::Model::DecisionTree *tree_ref)
{
    delete tree_ref;
    return;
}



    /* DEPRECATED: This is the old hand-rolled model encoding format.
     *
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
    */

