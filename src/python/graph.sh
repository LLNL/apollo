#!/bin/bash
rm -f ./model.png
dot -Tpng model.dot > model.png
display ./model.png
