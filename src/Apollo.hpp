
#include "raja.h"
#include "sos.h"
#include "sos_types.h"


#define APOLLO_VERBOSE 1


extern "C" {
    // SOS will delivery triggers and query results here:
    static SOS_feedback_handler_f
    handleFeedback(int msg_type,
                   int msg_size,
                   void *data);
}

class Apollo
{
    public:
        Apollo();
        ~Apollo();
        
        //Function ideas:
        //  setLearnMode()
        //  getLearnMode()
        //  double getHysterisis()
        //  requestDecisionTree()
        //  generateMultiPolicy()
        //  
        // 

        //NOTE: Eventually, let's disallow certain operators.
        //Apollo(const Apollo&) = delete;
        //Apollo& operator=(const Apollo&) = delete;
        
    private:
        SOS_runtime     *sos;
        SOS_pub         *pub;
};


