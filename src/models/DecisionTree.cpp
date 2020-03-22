
// Copyright (c) 2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include <sys/time.h> //ggout

#include "apollo/Apollo.h"
#include "apollo/models/DecisionTree.h"

#define modelName "decisiontree"
#define modelFile __FILE__


using namespace std;


double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + tv.tv_usec / 1e6);
}

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
    //double t1 = get_time(); //ggout
    //
    
    Node *iter;
    for(iter=node; !iter->is_leaf; ) {
        double feat_val = apollo->features[iter->feature_index].value;
        if (feat_val <= iter->value_LEQ)
            iter = iter->left_child;
        else
            iter = iter->right_child;
    }

    return iter->recommendation;
} //end: recursiveTreeWalk(...)   [function]


int
Apollo::Model::DecisionTree::getIndex(void)
{
    // Keep choice around for easier debugging, if needed:
    static int choice = -1;

    // Find the recommendation of the decision tree:
    choice = recursiveTreeWalk(tree_head);

    return choice;
}


void
Apollo::Model::DecisionTree::configure(
        int  numPolicies)
{
    // Construct a decisiontree for this model_definition.

    apollo       = Apollo::instance();
    policy_count = numPolicies;

    // Recursive function that constructs tree from nested JSON:
    //double t1 = get_time();
    // TODO: fix the DTree
    //tree_head = nodeFromJson(model_def, nullptr, 1);
    //std::cout << "Configure model " << (get_time() - t1 ) << " seconds" << std::endl; //ggout

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
    name = "DecisionTree";
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



