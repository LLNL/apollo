import numpy as np

from sklearn.cross_validation import cross_val_score, KFold, train_test_split
from sklearn.metrics import f1_score, make_scorer, accuracy_score


def get_kfold_score(classifier, X, y, folds, scoring):
    kf = KFold(len(y), n_folds=folds, shuffle=True)

    if scoring == 'f1':
        scoring = make_scorer(f1_score, greater_is_better=True, pos_label=None, average='weighted')

    accuracy = cross_val_score(classifier, X, y, cv=kf, scoring=scoring, n_jobs=-1)
    return accuracy.mean()


def get_kfold_pem_score(classifier, fX, X, y, folds, scoring):
    test_percentage = 0.1
    accuracy = []

    for i in range(folds):
        X_train, X_test, y_train, y_test = train_test_split(
                X, y, test_size=test_percentage)

        classifier.fit(X_train, y_train)
        accuracy.append(scoring(classifier, fX, X, y))

    return np.mean(accuracy)


def get_pem_time(classifier, fX, X, y, scoring):
    test_percentage = 0.1

    X_train, X_test, y_train, y_test = train_test_split(
            X, y, test_size=test_percentage)

    classifier.fit(X_train, y_train)
    return scoring(classifier, fX, X, y)


def get_cross_app_score(clf, train_X, train_y, test_X, test_y, scoring):
    clf.fit(train_X, train_y)
    y_pred = clf.predict(test_X)

    if scoring == 'f1':
        return f1_score(test_y, y_pred, pos_label=None, average='weighted')
    elif scoring == 'accuracy':
        return accuracy_score(test_y, y_pred)

def get_cross_app_pem_score(clf, test_fX, train_X, train_y, test_X, test_y, folds, scoring):
    clf.fit(train_X, train_y)
    return scoring(clf, test_fX, test_X, test_y)
