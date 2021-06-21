#ifndef APOLLO_MODELS_OPTIMAL_H
#define APOLLO_MODELS_OPTIMAL_H

#include "apollo/PolicyModel.h"

#include <string>
#include <deque>

class Optimal : public PolicyModel {
    public:
      Optimal(std::string file);
      ~Optimal(){};

      //
      int getIndex(std::vector<float> &features);
      void store(const std::string &filename){};

    private:
        std::deque<int> optimal_policy;

}; //end: Optimal (class)


#endif
