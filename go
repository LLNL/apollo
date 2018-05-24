rm -rf ./build
rm -rf ./install
mkdir build
mkdir install
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install -DSOS_DIR=$SOS_DIR -DRAJA_DIR=$(spack location -i raja) -DCALIPER_DIR=$(spack location -i caliper) ..
cd build
make && make install && cd .. && tree install

