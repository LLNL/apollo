#!/bin/bash
# Generate model
rm -f ./model.png
dot -Tpng model.dot > model.png
# Generate prediction tree
rm -f ./regress.png
dot -Tpng regress.dot > regress.png
# Show
display ./regress.png
