#!/usr/bin/env python
import numpy  as np
import pandas as pd    
from sklearn.preprocessing   import StandardScaler   
from sklearn.tree            import DecisionTreeClassifier
from sklearn.pipeline        import Pipeline
from sklearn.model_selection import cross_val_score
from sklearn.svm             import SVC

def main():

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
    dtree_pipe = [('standardize', StandardScaler()),\
                 ('estimator',    DecisionTreeClassifier(max_depth=5))]
    svc_pipe =   [('standardize', StandardScaler()),\
                 ('estimator',    SVC())]

    print ">>>>> Selecting pipe:"
    pipe = dtree_pipe
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
    # NOTE: Works specifically for decision tree.
    tree_to_code(trained_model, data.columns[:-1])



#########

from sklearn.tree import _tree

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
