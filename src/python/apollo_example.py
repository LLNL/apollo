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

    #####
    #
    #  Get the list of field names for non-string values.
    #

    this_frame = 0

    sql_string = """
        SELECT * FROM (
            SELECT
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.attr.level"
                                  THEN tblVals.val END) AS "cali_event_attr_level", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "vector_size"
                                  THEN tblVals.val END) AS "vector_size", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.end"
                                  THEN tblVals.val END) AS "cali_event_end", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "iteration"
                                  THEN tblVals.val END) AS "iteration", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "sum#time.inclusive.duration"
                                  THEN tblVals.val END) AS "sum#time_inclusive_duration", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "event.end#loop"
                                  THEN tblVals.val END) AS "event_end#loop" 
            FROM   tblPubs 
                  LEFT OUTER JOIN tblData 
                               ON tblPubs.guid = tblData.pub_guid 
                  LEFT OUTER JOIN tblVals 
                               ON tblData.guid = tblVals.guid 
            GROUP  BY tblVals.meta_relation_id
        ) WHERE cali_event_end IS NOT NULL;
    """
    results, col_names = SOS.query(sql_string, sos_host, sos_port)


    print ""
    print "=========="
    for row in results:
        print str(row)
    print "=========="
    for col in col_names:
        print str(col) + " "
    print "=========="
 
    ####


#### END
if __name__ == "__main__":
    ApolloExample()

