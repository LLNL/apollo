# Assumes Apollo built and installed under ../build/install

set -x

c++ -O3 -std=c++14 -I ../build/install/include -L ../build/install/lib \
    -Wl,-rpath,../build/install/lib apollo-test.cpp -o ./apollo-test.x -lapollo
