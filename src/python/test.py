#!/usr/bin/env python

import json
from pprint import pprint

with open('mytree.json') as tree:
    t = json.load(tree)
    

pprint(t)
