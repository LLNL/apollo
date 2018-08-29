#!/usr/bin/env python
import os
import sys
import pprint as pp
from ssos import SSOS

def ApolloExample():
    
    SOS = SSOS()
    SOS.init()

    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    sql_string = """
            SELECT
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "featureName"
                                  THEN tblVals.val END) AS "featureName",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "featureHint"
                                  THEN tblVals.val END) AS "featureHint",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "featureGoal"
                                  THEN tblVals.val END) AS "featureGoal",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "featureUnit"
                                  THEN tblVals.val END) AS "featureUnit",
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "featureValue"
                                  THEN tblVals.val END) AS "featureValue"
            FROM   tblPubs 
                  LEFT OUTER JOIN tblData 
                               ON tblPubs.guid = tblData.pub_guid 
                  LEFT OUTER JOIN tblVals 
                               ON tblData.guid = tblVals.guid 
            GROUP  BY
                tblVals.meta_relation_id 
        ;
    """

    print sql_string;

    results, col_names = SOS.query(sql_string, sos_host, sos_port)


    #    ) WHERE cali_event_end IS NOT NULL;
    #

    print ""
    print "=========="
    print str(col_names)
    print "----------"
    for row in results:
        print str(row)
    print "=========="
 
    ####


#### END
if __name__ == "__main__":
    ApolloExample()

