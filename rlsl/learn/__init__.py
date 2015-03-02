from sklearn.cross_validation import cross_val_score, KFold

def get_mean_accuracy(classifier, X, y, folds, scoring):
    kf = KFold(len(y), n_folds=folds, shuffle=True)

    accuracy = cross_val_score(classifier, X, y, cv=kf, scoring=scoring,n_jobs=-1)

    return accuracy.mean()
