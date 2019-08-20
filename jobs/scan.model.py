#!/usr/bin/env python

import json


def main():
    print("loading 'model.previous' ...")
    model_data = json.load(open("model.previous"))
    print("this model contains information on %d regions."
            % len(model_data["region_names"]))


    #
    #
    return



if __name__ == "__main__":
    main()
