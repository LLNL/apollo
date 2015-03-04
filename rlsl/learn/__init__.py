from sklearn.cross_validation import cross_val_score, KFold
from sklearn.metrics import f1_score, make_scorer

def get_kfold_score(classifier, X, y, folds, scoring):
    kf = KFold(len(y), n_folds=folds, shuffle=True)

    if scoring == 'f1':
        scoring = make_scorer(f1_score, greater_is_better=True, pos_label=None, average='weighted')

    print scoring
    accuracy = cross_val_score(classifier, X, y, cv=kf, scoring=scoring,n_jobs=-1)
    #accuracy = cross_val_score(classifier, X, y, cv=kf, scoring=scoring)#,n_jobs=-1)

    return accuracy.mean()
