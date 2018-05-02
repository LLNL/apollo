


def generate_sql:
    #####
    #
    #  Get the list of field names for non-string values.
    #

    this_frame = 0

    apollo_fields = """
    SELECT
    DISTINCT value_name 
    FROM viewCombined
    WHERE value_type != 3
    AND pub_title LIKE ""ApolloFeatures%""
    ;
    """
    results, col_names = SOS.query(sql_string,
            "localhost", 
            os.environ.get("SOS_CMD_PORT"))
    numeric_fields = dict()
    numeric_fields['name'] = [el[0] for el in results]

    name_count = len(numeric_fields['name'])
    print str(name_count) + " unique names."
    #
    #####

    filenames = [] 
    #####
    #
    #  Compose a query with the unique numeric fields as columns:
    #
    sql_string  = """ """
    sql_string += """ SELECT """
    sql_string += """ comm_rank """
    # sql_string += """,frame """
    for field_name in numeric_fields['name']:
        sql_string += """,GROUP_CONCAT( CASE WHEN """
        sql_string += ' value_name LIKE "' + field_name + '" '
        sql_string += ' THEN value END) AS "' + field_name + '" '
    sql_string += """, GROUP_CONCAT( CASE WHEN """
    sql_string += ' value_name LIKE "APOLLO.exec_time" '
    sql_string += ' THEN value END) AS "APOLLO.exec_time" '
    sql_string += """ FROM viewCombined """
    sql_string += " WHERE frame = " + str(this_frame) + " " 
    sql_string += """ GROUP BY """
    sql_string += """ comm_rank """
    # sql_string += """,frame """
    sql_string += """;"""
    #print "Composite SQL statement: "
    #print sql_string
    #print ""
    #print "Running composite query..."
    results, col_names = SOS.query(sql_string,
            "localhost",
            os.environ.get("SOS_CMD_PORT"))
    #print ""
    #
    #  Print out the results:
    #
    #print "=========="
    #for col in col_names:
    #    print str(col) + " "
    #print "=========="
    #for row in results:
    #    for col_index in range(len(row)):
    #        print str(col_names[col_index]) + ": " + str(row[col_index])
    #    print "----------"
    #print "=========="
    #
    #####


    #####
    #
    #  Build an attribute dictionary of the values.
    #
    attr = dict()
    attr['comm_rank']  =  [el[0] for el in results]

    position = 1
    for field_name in numeric_fields['name']:
        attr[field_name] = [el[position] for el in results]
        #print str(field_name) + " in position " + str(position) + " = " + str(attr[field_name])
        position += 1
    res_coords = [el[position] for el in results]
    #print "lulesh.coords in position " + str(position) + " = " + str(res_coords)

    #for field_name in numeric_fields['name']:
    #    rank = 0
    #    for this_ranks_value in attr[field_name]:
    #        print "comm_rank(" + str(rank) + ")." + field_name + " = " + this_ranks_value
    #        rank += 1
    #####

