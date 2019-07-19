
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
    if (node->feature->value <= node->value_LEQ) {
        if (node->left_child == nullptr) {
            return node->recommendation;
        } else {
            return recursiveTreeWalk(node->left_child);
        }
    }
    if (node->feature->value > node->value_LEQ) {
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
        int  numPolicies,
        json model_definition)
{

    apollo       = Apollo::instance();
    policy_count = numPolicies;

    // Construct a decisiontree for this model_definition.
    if ((model_definition == nullptr) || (model_definition == NULL)) {
        fprintf(stderr, "[WARNING] Cannot successfully configure"
                " with a NULL or empty model definition.\n");
        model_def = nullptr;
        configured = false;
        return;
    }

    log("DecisionTree::configure() called with the following model_definition: \n",
            model_definition);

    model_def = model_definition;

    // ----------
    // Recursive function that constructs tree from nested JSON:
    tree_head = nodeFromJson(model_def, nullptr, 1);
    configured = true;

    return;
}

    // NOTE: Deprecated from prior incarnation of model as a string. It stays a JSON object now.
    //json j;
    //try {
    //    j = json::parse(model_def);
    //} catch (json::parse_error& e) {
    //    // output exception information
    //    std::cout << "Error parsing JSON:\n\tmessage: " << e.what() << '\n'
    //        << "\texception id: " << e.id << '\n'
    //        << "\tbyte position of error: " << e.byte << std::endl;
    //    exit(EXIT_FAILURE);
    //}


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

    // [ ] IF there is a "rule", there are L/R children
    if (j.find("rule") != j.end()) {
        // [ ] We're a branch
        node->is_leaf = false;
        // [ ] Extract the feature name and the comparison value for LEQ/GT
        cali_id_t   feat_id;

        std::string rule = j["rule"].get<string>();
        std::string feat_name = "";
        std::string comparison_symbol = "";
        double      leq_val = 0.0;

        std::stringstream rule_split(rule);

        rule_split >> feat_name;
        rule_split >> comparison_symbol;
        rule_split >> leq_val;

        //log("feat_name == '", feat_name, "'");
        //log("comparison_symbol == '", comparison_symbol, "'");
        //log("leq_val == '", leq_val, "'");

        node->value_LEQ = leq_val;

        // Scan to see if we have this feature in our accelleration structure::
        bool found = false;
        Feature *feat;

        for (auto f : tree_features) {
            if (f->name == feat_name) {
                feat = f;
                found = true;
                break;
            }
        }
        if (not found) {
            // add it
            feat_id = cali_find_attribute(feat_name.c_str());
            if (feat_id == CALI_INV_ID) {
                fprintf(stderr, "== APOLLO: "
                "[ERROR] DecisionTree refers to features not present in Caliper data.\n"
                "\tThis is likely due to an error in the Apollo Controller logic.\n");
                fprintf(stderr, "== APOLLO: Referenced feature: \"%s\"\n",
                        feat_name.c_str());
                fflush(stderr);
                exit(EXIT_FAILURE);
            } else {
                feat = new Feature();
                // NOTE: feat->value_variant and feat->value are filled
                //       before being used for traversal in getIndex()
                feat->cali_id = feat_id;
                feat->name    = feat_name;
                tree_features.push_back(feat);
            }
        }
        node->feature = feat;

        log(indent, "if (", feat_name, " <= ", node->value_LEQ, ")");

        // [ ] Recurse into the L/R children
        node->left_child = nodeFromJson(j["left"],   node, my_indent + 1);
        log(indent, "else");
        node->right_child = nodeFromJson(j["right"], node, my_indent + 1);
    } else {
        // [ ] We are a leaf, extract the values from the tree
        node->is_leaf = true;
        node->feature = node->parent_node->feature;
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
        for (Feature *feat : tree_features) {
            if (feat != nullptr) { delete feat; }
        }
        tree_features.clear();
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

