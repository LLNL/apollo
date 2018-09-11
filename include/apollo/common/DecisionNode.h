
#ifndef APOLLO_COMMON_DECISIONNODE_H
#define APOLLO_COMMON_DECISIONNODE_H

#include "apollo/Apollo.h"

template <typename T, typename NVAL>
class Apollo::DecisionNode {
    // NOTE: This is the low-level building block for the tree.
    //       The actual tree is built and searched using the
    //       src/models/DecisionTree.cpp code.
    public:
        DecisionNode(Apollo *apollo_ptr,
                     DecisionNode *parent_ptr,
                     const char *feature_name,
                     CMPR leq_val,
                     CMPR gt_val,
                     NVAL node_value);
        ~DecisionNode();

    private:
        Apollo *apollo = nullptr;
        //
        Apollo::DecisionNode *leq_node    = nullptr;
        Apollo::DecisionNode *gt_node     = nullptr;
        Apollo::DecisionNode *parent_node = nullptr;
        //
        CMPR leq_val;
        CMPR gt_val;
        NVAL node_value;

}; // end: Apollo::DecisionNode



#endif

