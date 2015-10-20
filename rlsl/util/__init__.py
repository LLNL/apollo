import os
import datetime

def df_to_unique_csv(df, file_prefix):
    now = datetime.datetime.now()
    d = '%s-%s-%s' % (now.year, now.month, now.day) 

    if not os.path.exists(d):
        os.makedirs(d)

    i = 0
    while os.path.exists("%s/%s.%s.csv" % (d, file_prefix, i)):
        i += 1

    df.to_csv(path_or_buf='%s/%s.%s.csv' % (d, file_prefix, i), index=False)
