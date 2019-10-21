#!/bin/bash
cd build
make -j 8 && make install && cd .. && tree install

