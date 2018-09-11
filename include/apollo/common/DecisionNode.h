
#ifndef APOLLO_COMMON_DECISIONNODE_H
#define APOLLO_COMMON_DECISIONNODE_H

#include "apollo/Apollo.h"

template <typename CMP_T, VAL_T>
class Apollo::DecisionNode {
    // NOTE: This is the low-level building block for the tree.
    //       The actual tree is built and searched using the
    //       src/models/DecisionTree.cpp code.
    public:
        DecisionNode(
                Apollo     *apollo_ptr,
                int         node_id_int,
                const char *feature_name_str)
        {
            apollo       = apollo_ptr;
            node_id      = node_id_int;
            if (feature_name_str != NULL) {
                feature_name = std:string(feature_name_str);
            } else {
                feature_name = "UNDEFINED";
            }
        }
        ~DecisionNode();

        bool    filled = false;
        //
        Apollo::DecisionNode *parent_node = nullptr;
        int     node_id;
        //
        std::string feature_name;
        //
        Apollo::DecisionNode *leq_child_node = nullptr;
        Apollo::DecisionNode *grt_child_node = nullptr;
        //
        CMP_T leq_val;
        CMP_T grt_val;
        VAL_T node_val;


    private:
        Apollo *apollo = nullptr;

}; // end: Apollo::DecisionNode  (class template)



#endif

