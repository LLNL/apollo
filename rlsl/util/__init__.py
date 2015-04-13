import os

def df_to_unique_csv(df, file_prefix):
    i = 0
    while os.path.exists("%s.%s.csv" % (file_prefix, i)):
        i += 1

    df.to_csv(path_or_buf='%s.%s.csv' % (file_prefix, i), index=False)
