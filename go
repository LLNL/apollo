#!/bin/bash
cd build
make -j && make install && cd .. && tree install

# rm -f out_raw.txt
# rm -f out.txt
# make -j &> out_raw.txt && make install && cd .. && tree install
# cat out_raw.txt | sed -r "s/\x1B\[([0-9]{1,2}(;[0-9]{1,2})?)?[mGK]//g" > out.txt
# cat out_raw.txt
# rm out_raw.txt
