#!/bin/bash
cd build
make -j && make install && cd .. && tree install

