# Assumes apollo has been built in ../build, installed in ../build/install.
# Assumes hipcc in the path shown.

/opt/rocm-4.5.2/bin/hipcc -I ../build/install/include -L ../build/install/lib \
    -Wl,-rpath,../build/install/lib test-hip.cpp -o test-hip.x -lapollo -ldl
