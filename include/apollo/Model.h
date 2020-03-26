#ifndef APOLLO_MODEL_H
#define APOLLO_MODEL_H

#include <string>
#include <vector>

// Abstract
class Model {
    public:
        Model() {};
        Model(int num_policies, std::string name, bool training) : 
            name(name),
            training(training),
            policy_count(num_policies)
        {};
        virtual ~Model() {};
        //
        virtual int      getIndex(std::vector<float> &features) = 0;

        virtual void    store(const std::string &filename) {};

        std::string      name           = "";
        bool             training       = false;
    protected:
        //
        int          policy_count;
}; //end: Model (abstract class)


#endif
