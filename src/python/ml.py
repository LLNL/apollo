#!/usr/bin/env python
import numpy  as np
import pandas as pd    
import cStringIO
from sklearn.preprocessing   import StandardScaler   
from sklearn.tree            import DecisionTreeClassifier
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.svm             import SVC

from ssos import SSOS 

def main():

    sos = SSOS()

    # print "numpy.__version__   == " + str(np.__version__)
    # print "pandas.__version__  == " + str(pd.__version__)
    # print "sklearn.__version__ == " + str(skl.__version__)

    data = pd.read_csv("kripke_sample.csv", index_col=False)

    # Target
    # NOTE: Here binned into every 10% for DecisionTreeClassifier 
    y = (data["percent_of_fastest"].values/10).astype(int)
    y = y.squeeze()

    # Features
    x = data.drop("percent_of_fastest", axis="columns").values

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

    print ">>>>> Cross-validation... (10-fold)"
    scores = cross_val_score(model, x, y, cv=10)
    print("\n".join([("    " + str(score)) for score in scores]))
    print "    score.mean == " + str(np.mean(scores))

    print ">>>>> Training model..."
    model.fit(x, y)

    trained_model = model.named_steps["estimator"]
    print trained_model


    print ">>>>> Rules learned:"
    rules = tree_to_string(trained_model, data.columns[:-1])

    print rules


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
        offset = "  " * depth
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            result_str.write("{} : {}{} <= {}\n".format(depth, offset, name, threshold))
            recurseSTR(result_str, tree_.children_left[node], depth + 1)
            result_str.write("{} : {}{} > {}\n".format(depth, offset, name, threshold))
            recurseSTR(result_str, tree_.children_right[node], depth + 1)
        else:
            #result_str.write("INDEX EQ {}\n".format(tree_.value[node]))
            result_str.write("{} : {}return = {}\n".format(depth, offset, tree_.value[node]))
    
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
