#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>

#include "apollo/Apollo.h"
#include "apollo/Model.h"

class Apollo::Model::DecisionTree : public Apollo::Model {

    public:
        DecisionTree();
        ~DecisionTree();

        void configure(int num_policies);

        int  getIndex(void);

    private:

        class Node {
            public:
                Node() {};
                ~Node() {};

                bool        is_leaf;

                int         recommendation;
                std::vector<float>
                            recommendation_vector;

                double            value_LEQ;
                int               feature_index;
                Node             *left_child;
                Node             *right_child;
                Node             *parent_node;

                int         indent;
        }; // end: Node (class)

        int  recursiveTreeWalk(Node *node);

        Node                   *tree_head;
        std::vector<Node *>     tree_nodes;

}; //end: Apollo::Model::DecisionTree (class)


#endif
