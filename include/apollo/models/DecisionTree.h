#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#include <string>
#include <vector>
#include <map>

#include "caliper/cali.h"
#include "caliper/common/cali_types.h"

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::DecisionTree : public Apollo::ModelObject {

    public:
        DecisionTree();
        ~DecisionTree();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                std::string model_definition);
    
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
                Node(Apollo *apollo_ptr, Feature *feat_ptr)
                {
                    apollo       = apollo_ptr;
                    //
                    parent_node  = nullptr;
                    left_child   = nullptr;
                    right_child  = nullptr;
                    //
                    feature      = feat_ptr;
                }
                ~Node() {};

                double    value_LEQ;
                double    value_GRT;
                int       recommendation;

                Feature  *feature;
                Node     *left_child;
                Node     *right_child;


            private:
                Apollo   *apollo;
                //
                bool      filled = false;
                //
                Node     *parent_node;
                //
                //
        }; // end: Node (class)
        //
        int  recursiveTreeWalk(Node *node);
        //
        Node                   *tree_head;
        std::vector<Node *>     tree_nodes;
        std::vector<Feature *>  tree_features;

}; //end: Apollo::Model::DecisionTree (class)


#endif
