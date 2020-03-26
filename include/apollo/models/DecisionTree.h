#ifndef APOLLO_MODELS_DECISIONTREE_H
#define APOLLO_MODELS_DECISIONTREE_H

#include <string>
#include <vector>

#include "apollo/Model.h"

class DecisionTree : public Model {

    public:
        DecisionTree(int num_policies, std::vector< std::vector<float> > &features, std::vector<int> &responses);

        ~DecisionTree();

        int  getIndex(void);
        int  getIndex(std::vector<float> &features);
        void store(const std::string &filename);

    private:
}; //end: DecisionTree (class)


#endif
