#!/usr/bin/env python
import os
import numpy  as np
import pandas as pd    
import sklearn as skl
import cStringIO
from sklearn.preprocessing   import StandardScaler   
from sklearn.tree            import DecisionTreeClassifier
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.svm             import SVC

from ssos import SSOS 

sos = SSOS() 

def main():

    sos.init()

    sos_host = "localhost"
    sos_port = os.environ.get("SOS_CMD_PORT")

    # Submit the query to SOS, return results as a DataFrame:
    data = getTrainingData(sos_host, sos_port, row_limit=0);
    
    # Remove the redundant stuff from Caliper:
    data = data[data.event_end_loop != "NULL"]

    # Process an example ML pipeline w/hardcoded field names:
    learningExample(data)

    return

#########


def tablePrint(results):
    # Print out the results in a pretty column-aligned way:
    widths = [max(map(len, str(col))) for col in zip(*results)]
    for row in results: 
        print "  ".join((val.ljust(width) for val, width in zip(row, widths)))
    #
    return



def getTrainingData(sos_host, sos_port, row_limit):

    sql_string = """
        SELECT * FROM (
            SELECT
                  tblVals.frame,
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.attr.level"
                                  THEN tblVals.val END) AS "cali_event_attr_level", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "vector_size"
                                  THEN tblVals.val END) AS "vector_size", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "cali.event.end"
                                  THEN tblVals.val END) AS "cali_event_end", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "loop"
                                  THEN tblVals.val END) AS "loop", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "policyIndex"
                                  THEN tblVals.val END) AS "policyIndex", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "sum#time.inclusive.duration"
                                  THEN tblVals.val END) AS "sum_time_inclusive_duration", 
                  GROUP_CONCAT(CASE WHEN tblData.NAME LIKE "event.end#loop"
                                  THEN tblVals.val END) AS "event_end_loop" 
            FROM   tblPubs 
                  LEFT OUTER JOIN tblData 
                               ON tblPubs.guid = tblData.pub_guid 
                  LEFT OUTER JOIN tblVals 
                               ON tblData.guid = tblVals.guid 
            GROUP  BY tblVals.meta_relation_id 
        )   WHERE  cali_event_end IS NOT NULL
    """

    if (row_limit < 1):
        sql_string += ";"
    else:
        sql_string += "LIMIT " + str(row_limit) + ";"

    results, col_names = sos.query(sql_string, sos_host, sos_port)
    data = pd.DataFrame.from_records(results, columns=col_names)
   
    print str(data["sum_time_inclusive_duration"].values)

    return data


def learningExample(data): 
    print "numpy.__version__   == " + str(np.__version__)
    print "pandas.__version__  == " + str(pd.__version__)
    print "sklearn.__version__ == " + str(skl.__version__)

    # We're commenting this out since we pass in a DataFrame:
    # data = pd.read_csv("kripke_sample.csv", index_col=False)

    # NOTE: These fields are in the example data.
    #cali_event_attr_level
    #vector_size
    #cali_event_end
    #loop
    #policyIndex
    #sum_time_inclusive_duration
    #event_end_loop

    # Target
    # NOTE: Here binned into every 10% for DecisionTreeClassifier 

    print ">>>>> Extracting dependent variable for Y-axis..."
    y = (data["sum_time_inclusive_duration"].values.astype(float).astype(int)/5).astype(int)
    y = y.squeeze()

    # Extract relevant independent features:
    x = data.drop([
            "cali_event_attr_level",
            "cali_event_end",
            "sum_time_inclusive_duration",
            "loop",
            "event_end_loop",
            "frame"
        ], axis="columns").values

    print ">>>>> Initializing pipelines..."
    basic_pipe = [('estimator',   DecisionTreeClassifier(max_depth=5))]
    dtree_pipe = [('standardize', StandardScaler()),\
                 ('estimator',    DecisionTreeClassifier(max_depth=5))]
    svc_pipe =   [('standardize', StandardScaler()),\
                 ('estimator',    SVC())]

    print ">>>>> Selecting pipe:"
    pipe = basic_pipe
    print pipe

    print ">>>>> Initializing model..."
    model = Pipeline(pipe)

    print ">>>>> Cross-validation... (5-fold)"
    scores = cross_val_score(model, x, y, cv=5)
    print("\n".join([("    " + str(score)) for score in scores]))
    print "    score.mean == " + str(np.mean(scores))

    print ">>>>> Training model..."
    model.fit(x, y)

    trained_model = model.named_steps["estimator"]

    print ">>>>> Rules learned:"
    rules = tree_to_string(trained_model, ["vector_size", "policyIndex"])
    # tree_to_code(trained_model, ["vector_size", "policyIndex"])

    print rules

    return



#########

from sklearn.tree import _tree

def tree_to_string(tree, feature_names):
    result = cStringIO.StringIO()
    tree_ = tree.tree_
    feature_name = [
        feature_names[i] if i != _tree.TREE_UNDEFINED else "undefined!"
        for i in tree_.feature
    ]
    result.write("{}\n".format(str(len(feature_names))))
    for feature in feature_names:
        result.write("{}\n".format(feature))
   
    print "Recursing..."
    def recurseSTR(result_str, node, depth):
        # NOTE: Useful for debugging trees:
        #       offset = "  " * depth
        #
        offset = ""
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            result_str.write("{} : {}{} <= {}\n".format(depth, offset, name, threshold))
            recurseSTR(result_str, tree_.children_left[node], depth + 1)
            result_str.write("{} : {}{} > {}\n".format(depth, offset, name, threshold))
            recurseSTR(result_str, tree_.children_right[node], depth + 1)
        else:
            #result_str.write("INDEX EQ {}\n".format(tree_.value[node]))
            result_str.write("{} : {}policyIndex = 99.99\n".format(depth, offset))
    
    recurseSTR(result, 0, 1)
    return result.getvalue()


def tree_to_code(tree, feature_names):
    tree_ = tree.tree_
    feature_name = [
        feature_names[i] if i != _tree.TREE_UNDEFINED else "undefined!"
        for i in tree_.feature
    ]
    print "def tree({}):".format(", ".join(feature_names))

    def recurse(node, depth):
        indent = "  " * depth
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            print "{}if {} <= {}:".format(indent, name, threshold)
            recurse(tree_.children_left[node], depth + 1)
            print "{}else:  # if {} > {}".format(indent, name, threshold)
            recurse(tree_.children_right[node], depth + 1)
        else:
            print "{}return {}".format(indent, tree_.value[node])

    recurse(0, 1)


if __name__ == "__main__":
    main()
