#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::DecisionTree : public Apollo::ModelObject {

    public:
        DecisionTree();
        ~DecisionTree();

        void configure(
                Apollo     *apollo_ptr,
                int         num_policies,
                const char *model_definition);
        //
        int  getIndex(void);

    private:
        class Node {
            public:
                Node(
                        Apollo     *apollo_ptr,
                        int         node_id_int,
                        const char *feature_name_str)
                {
                    apollo       = apollo_ptr;
                    node_id      = node_id_int;
                    if (feature_name_str != NULL) {
                        feature_name = feature_name_str;
                    } else {
                        feature_name = "UNDEFINED";
                    }
                    //
                    parent_node    = NULL;
                    leq_child_node = NULL;
                    grt_child_node = NULL;
                }
                ~Node() {
                    feature_name = "";
                };

                void  setLeqVal(double leq_val_fill) { leq_val = leq_val_fill; }
                double getLeqVal(void) { return leq_val; }

                void  setGrtVal(double grt_val_fill) { grt_val = grt_val_fill; }
                double getGrtVal(void) { return grt_val; }

                void  setNodeVal(int node_val_fill) { node_val = node_val_fill; }
                int getNodeVal(void) { return node_val; }

            private:
                Apollo   *apollo;
                //
                bool      filled = false;
                //
                double    leq_val;
                double    grt_val;
                int       node_val;
                //
                Node     *parent_node;
                int       node_id;
                //
                std::string feature_name;
                //
                Node *leq_child_node;
                Node *grt_child_node;
                //
        }; // end: Node (class)
        //
        Node                   *tree_head;
        std::map<int, Node *>   tree_nodes;
        std::list<std::string>  tree_features;


}; //end: Apollo::Model::DecisionTree (class)


#endif
