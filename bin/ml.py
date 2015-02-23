import numpy as np

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

import csv
from sklearn.feature_extraction import DictVectorizer
from sklearn.externals.six import StringIO
from sklearn.cross_validation import cross_val_score, train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.utils import shuffle

features_file = '/g/g92/ukbeck/Projects/RAJA/test/LULESH-v1.0/learner/X.csv'
labels_file = '/g/g92/ukbeck/Projects/RAJA/test/LULESH-v1.0/learner/y.csv'
timing_file = '/g/g92/ukbeck/Projects/RAJA/test/LULESH-v1.0/learner/times.csv'

features = []
labels = []

reader = csv.reader(open(features_file, 'rb'), delimiter=',')
parsed = ((row[0], row[1], int(row[2])) for row in reader)
for row in parsed:
  features.append(row)

reader = csv.reader(open(labels_file, 'rb'), delimiter=',')
for row, record in enumerate(reader):
  labels.append(record)

features = [dict(zip(['Set Type', 'Loop', 'Size', 'Segments'], f)) for f in features]
labels = [dict(zip(['Tile', 'Outer', 'Inner'], l)) for l in labels]

feature_vect = DictVectorizer(sparse=False)
label_vect = DictVectorizer(sparse=False)

X = feature_vect.fit_transform(features)
y = label_vect.fit_transform(labels)

X, y = shuffle(X,y)

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size = 0.3)

#for x in X_test:
#  print x, feature_vect.inverse_transform(x)

#clf.fit(X_train, y_train)
#print "Test/train accuracy: %0.4f" % clf.score(X_test, y_test)

clf = RandomForestClassifier(n_estimators=15)
#scores = cross_val_score(clf, X, y, cv=20) 
#print "Cross-validated accuracy @ %d estimators: %0.4f (+/- %0.4f)" % (10, scores.mean(), scores.std() / 2)

clf.fit(X_train, y_train)
print "Test/train accuracy: %0.4f" % clf.score(X_test, y_test)

for x,y in zip(X_test, y_test):
  result = clf.predict(x)
  print label_vect.inverse_transform(result) == label_vect.inverse_transform(y)

importances = clf.feature_importances_
std = np.std([tree.feature_importances_ for tree in clf.estimators_],
             axis=0)

indices = np.argsort(importances)[::-1]

# Print the feature ranking
print("Feature ranking:")
for f in range(len(indices)):
    print("%d. feature %s (%f)" % (f + 1, feature_vect.get_feature_names()[indices[f]], importances[indices[f]]))

# Plot the feature importances of the forest
plt.figure()
plt.title("Feature importances")
plt.bar(range(len(indices)), importances[indices],
       color="r", yerr=std[indices], align="center")
plt.xticks(range(len(indices)), [feature_vect.get_feature_names()[x] for x in indices], rotation='vertical')
plt.xlim([-1, len(indices)])
plt.savefig('importance.pdf',bbox_inches='tight')
