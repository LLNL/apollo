#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "caliper/cali.h"
#include "caliper/common/cali_types.h"

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::DecisionTree : public Apollo::ModelObject {

    public:
        DecisionTree();
        ~DecisionTree();

        void configure(int num_policies, json model_definition);

        int  getIndex(void);

    private:
        class Feature {
            public:
                Feature() {};
                ~Feature() {};
                cali_id_t       cali_id;
                std::string     name;
                cali_variant_t  value_variant;
                double          value;
        };

        class Node {
            public:
                Node() {};
                ~Node() {};

                bool        is_leaf;

                int         recommendation;
                std::vector<float>
                            recommendation_vector;

                double      value_LEQ;
                Feature    *feature;
                Node       *left_child;
                Node       *right_child;
                Node       *parent_node;


                int         indent;
                int         json_id;

        }; // end: Node (class)


        Node* nodeFromJson(nlohmann::json parsed_json, Node *parent, int indent);
        int  recursiveTreeWalk(Node *node);

        Node                   *tree_head;
        std::vector<Node *>     tree_nodes;
        std::vector<Feature *>  tree_features;

}; //end: Apollo::Model::DecisionTree (class)


#endif
