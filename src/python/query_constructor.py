#!/usr/bin/env python
import os
import sys
import pprint as pp
from ssos import SSOS

def ApolloExample():
    DEBUG = False 
    
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
    SELECT
    DISTINCT tblData.name 
    FROM
        tblPubs
        LEFT OUTER JOIN tblData ON tblPubs.guid = tblData.pub_guid
    WHERE
        tblPubs.title LIKE "caliper.data"
    ;
    """
    results, col_names = SOS.query(sql_string, sos_host, sos_port)

    name_fields = dict()
    name_fields['name'] = [el[0] for el in results]
    name_count = len(name_fields['name'])

    if DEBUG == True:
        print str(name_count) + " caliper field names."
    #
    #####

    #####
    #
    #  Compose a query with the unique field names as columns:
    #
    sql_string  = """ """
    sql_string += """ SELECT """
    sql_string += """ tblVals.meta_relation_id """
    # sql_string += """,frame """
    for field_name in name_fields['name']:
        #if field_name == "cali.event.begin":
        #    continue
        #if field_name == "event.begin#loop":
        #    continue
        sql_string += """,GROUP_CONCAT( CASE WHEN """
        sql_string += ' tblData.name LIKE "' + field_name + '" '
        sql_string += ' THEN tblVals.val END) AS "' + str(field_name).replace(".", "_") + '" '
    #
    # NOTE: Add explicit stuff here, for example:
    #
    #sql_string += """, GROUP_CONCAT( CASE WHEN """
    #sql_string += ' tblData.name LIKE "APOLLO.exec_time" '
    #sql_string += ' THEN tblVals.value END) AS "APOLLO.exec_time" '
    #
    sql_string += """ FROM """
    sql_string += """    tblPubs """
    sql_string += """    LEFT OUTER JOIN tblData ON tblPubs.guid = tblData.pub_guid """
    sql_string += """    LEFT OUTER JOIN tblVals ON tblData.guid = tblVals.guid """
    sql_string += """ GROUP BY """
    sql_string += """ tblVals.meta_relation_id """
    #sql_string += """,tblVals.frame """
    sql_string += """;"""
    
    if DEBUG == True:
        print "Composite SQL statement: "
    print sql_string
    print ""
    print "Running composite query..."

    results, col_names = SOS.query(sql_string, sos_host, sos_port)


    if DEBUG == True:
        print ""
        print "=========="
        for col in col_names:
            print str(col) + " "
        print "=========="
        for row in results:
            for col_index in range(len(row)):
                print str(col_names[col_index]) + ": " + str(row[col_index])
            print "----------"
        print "=========="
    
    ####


#### END
if __name__ == "__main__":
    ApolloExample()

