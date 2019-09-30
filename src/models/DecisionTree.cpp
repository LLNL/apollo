
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/models/DecisionTree.h"

#define modelName "decisiontree"
#define modelFile __FILE__


using namespace std;


int
Apollo::Model::DecisionTree::recursiveTreeWalk(Node *node) {
    // Compare the node->value to the defined comparison values
    // and either dive down a branch or return the choice up.
    //
    // Every node needs to have an initialized FEATURE
    //
    //
    //IF this a LEAF node, return the int.
    //OTHERWISE...
    //
    double feat_val = apollo->features[node->feature_index].value;

    if (feat_val <= node->value_LEQ) {
        if (node->left_child == nullptr) {
            return node->recommendation;
        } else {
            return recursiveTreeWalk(node->left_child);
        }
    }
    if (feat_val > node->value_LEQ) {
        if (node->right_child == nullptr) {
            return node->recommendation;
        } else {
            return recursiveTreeWalk(node->right_child);
        }
    }
    return node->recommendation;
} //end: recursiveTreeWalk(...)   [function]


int
Apollo::Model::DecisionTree::getIndex(void)
{
    // Keep choice around for easier debugging, if needed:
    static int choice = -1;

    // NOTE: iter_count is here so we don't fill up stderr with endless warnings.
    iter_count++;
    if (configured == false) {
        if (iter_count < 10) {
            fprintf(stderr, "[ERROR] DecisionTree::getIndex() called prior to"
                " model configuration. Defaulting to index 0.\n");
            fflush(stderr);
        } else if (iter_count == 10) {
            fprintf(stderr, "[ERROR] DecisionTree::getIndex() has still not been"
                    " configured. Continuing default behavior without further"
                    " error messages.\n");
        }
        // Since we're not configured yet, return a default:
        choice = 0;
        return choice;
    }

    // Find the recommendation of the decision tree:
    choice = recursiveTreeWalk(tree_head);

    return choice;
}


void
Apollo::Model::DecisionTree::configure(
        int  numPolicies,
        json model_definition)
{
    // Construct a decisiontree for this model_definition.

    apollo       = Apollo::instance();
    policy_count = numPolicies;

    if ((model_definition == nullptr) || (model_definition == NULL)) {
        fprintf(stderr, "[WARNING] Cannot successfully configure"
                " with a NULL or empty model definition.\n");
        model_def = nullptr;
        configured = false;
        return;
    }
    log("DecisionTree::configure() called with the following model_definition: \n",
            model_definition);

    // Keep a copy of the JSON:
    model_def = model_definition;

    // Recursive function that constructs tree from nested JSON:
    tree_head = nodeFromJson(model_def, nullptr, 1);

    configured = true;
    return;
}


Apollo::Model::DecisionTree::Node*
Apollo::Model::DecisionTree::nodeFromJson(
        json j, Apollo::Model::DecisionTree::Node *parent, int my_indent)
{
    Apollo::Model::DecisionTree::Node *node =
        new Apollo::Model::DecisionTree::Node;
    tree_nodes.push_back(node);

    node->indent        = my_indent;
    node->parent_node   = parent;
    node->left_child    = nullptr;
    node->right_child   = nullptr;
    node->value_LEQ     = -1.0;

    std::string indent = std::string(my_indent * 4, ' ');

    // IF there is a "rule", there are L/R children
    if (j.find("rule") != j.end()) {
        // We're a branch
        node->is_leaf = false;
        // Extract the feature name and the comparison value for LEQ/GT

        std::string rule = j["rule"].get<string>();
        std::string feat_name = "";
        std::string comparison_symbol = "";
        double      leq_val = 0.0;

        std::stringstream rule_split(rule);

        rule_split >> feat_name;
        rule_split >> comparison_symbol;
        rule_split >> leq_val;

        node->value_LEQ = leq_val;

        // Find or Insert this feature:
        bool found = false;
        while (not found) {
            for (int i = 0; i < apollo->features.size(); ++i) {
                if (apollo->features[i].name == feat_name) {
                    node->feature_index = i;
                    found = true;
                    break;
                }
            }
            if (not found) {
                apollo->setFeature(feat_name, 0.0);
            }
        }

        log(indent, "if (", feat_name, " <= ", node->value_LEQ, ")");

        // [ ] Recurse into the L/R children
        node->left_child = nodeFromJson(j["left"],   node, my_indent + 1);
        log(indent, "else");
        node->right_child = nodeFromJson(j["right"], node, my_indent + 1);
    } else {
        // [ ] We are a leaf, extract the values from the tree
        node->is_leaf = true;
        node->feature_index = node->parent_node->feature_index;
        node->recommendation_vector = j["value"].get<vector<float>>();
        // [ ] Look at the vector and find the index with the most clients
        //     in it, that's where the best performing kernels were clustered
        //     at this point in the decision tree:
        bool    duplicate_maximums_exist = false;
        bool    more_than_one_best_fit   = false;
        float   val_here   = -1.0;
        float   max_seen   = -1.0;
        int     max_pos    = 0;
        int     pos        = 0;
        for (pos = 0; pos < node->recommendation_vector.size(); pos++) {
            val_here = node->recommendation_vector[pos];
            if ((val_here > 0) && (max_seen >= 0.0)) {
                more_than_one_best_fit = true;
            }
            if (val_here == max_seen) {
                // NOTE: This means our tree may be a bit confused here
                //       about what the best policy is for however some
                //       measurements were binned for analysis by the
                //       controller.
                // NOTE: Be default, we move to this as the new max_pos.
                duplicate_maximums_exist = true;
                max_pos = pos;
                // NOTE: We *could* get clever and look at the adjacent
                //       leaves and take the average, but for now, we don't.
            } else if (val_here > max_seen) {
                // We have a new winner!
                duplicate_maximums_exist = false;
                max_pos = pos;
                max_seen = val_here;
            }
        }
        //
        node->recommendation = max_pos;
        log(indent, "use kernel_variant ", node->recommendation,
                    (more_than_one_best_fit   ? " [>1 advised policy]" : ""),
                    (duplicate_maximums_exist ? " [duplicate maximum]" : ""));
    }

    return node;
}
//
// ----------
//
// BELOW: Boilerplate code to manage instances of this model:
//


Apollo::Model::DecisionTree::DecisionTree()
{
    name = "DecisionTree";
    iter_count = 0;
}

Apollo::Model::DecisionTree::~DecisionTree()
{
    if (configured == true) {
        configured = false;
        tree_head = nullptr;
        for (Node *node : tree_nodes) {
            if (node != nullptr) { delete node; }
        }
        tree_nodes.clear();
        model_def = "";
        iter_count = 0;
    }

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



