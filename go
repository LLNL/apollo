rm -rf ./build
rm -rf ./install
mkdir build
mkdir install
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install \
        -DSOS_DIR=/home/cdw/src/sos_flow/build \
        -DRAJA_DIR=$(spack location -i raja) \
        -DCALIPER_DIR=/home/cdw/src/caliper/build \
        ..
make && make install && cd .. && tree install
