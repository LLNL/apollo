# Assumes apollo has been built in ../build, installed in ../build/install

nvcc -x cu -g -Xlinker "-rpath,../build/install/lib" \
    -I ../build/install/include test-cuda.cpp -L ../build/install/lib -o test-cuda.x -lapollo -lcudart
